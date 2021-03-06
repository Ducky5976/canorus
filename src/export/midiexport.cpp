/*!
	Copyright (c) 2007-2020, Matevž Jekovec, Canorus development team
	All Rights Reserved. See AUTHORS for a complete list of authors.

	Licensed under the GNU GENERAL PUBLIC LICENSE. See LICENSE.GPL for details.
*/

#include <QFileInfo>
#include <QRegExp>
#include <QTextStream>
#include <iomanip>
#include <iostream>
#include <stdio.h>

#include "export/midiexport.h"

#include "interface/mididevice.h"
#include "interface/playback.h"
#include "score/document.h"
#include "score/sheet.h"
#include "score/staff.h"
#include "score/voice.h"

class CACanorus;

/*!
	\class CAMidiExport
	\brief Midi file export filter
	This class is used to export the document or parts of the document to a midi file.
	The most common use is to simply call one of the constructors
	\code
	CAMidiExport( myDocument, &textStream );
	\endcode

	\a textStream is usually the file stream.

	\sa CAMidiImport
*/

/*!
	Constructor for midi file export. Called when choosing the mid/midi file extension in the export dialog.
	Exports all voices to the given text stream.
*/
CAMidiExport::CAMidiExport(QTextStream* out)
    : CAExport(out)
    , CAMidiDevice()
{
    _midiDeviceType = MidiExportDevice;
    setRealTime(false);
    _trackTime = 0;
}

/*!
	Compute the time offset for a new event and update the current track time.
*/
int CAMidiExport::timeIncrement(int time)
{
    int offset = 0;
    if (time > _trackTime) {
        offset = time - _trackTime;
    }
    _trackTime = time;
    return offset;
}

void CAMidiExport::send(QVector<unsigned char> message, int time)
{
    if (message.size())
        trackChunk.append(writeTime(timeIncrement(time)));
    char q;
    for (int i = 0; i < message.size(); i++) {
        q = static_cast<char>(message[i]);
        trackChunk.append(q);
    }
    for (int i = 0; i < message.size(); i++) {
        q = static_cast<char>(message[i]);
    }
}

void CAMidiExport::sendMetaEvent(int time, char event, char a, char b, int)
{
    // We don't do a time check on time, and we compute
    // only the time increment when we really send an event out.
    QByteArray tc;
    if (event == CAMidiDevice::Meta_Keysig) {
        tc.append(writeTime(timeIncrement(time)));
        tc.append(static_cast<char>(CAMidiDevice::Midi_Ctl_Event));
        tc.append(event);
        tc.append(variableLengthValue(2));
        tc.append(a);
        tc.append(b);
        trackChunk.append(tc);
    } else if (event == CAMidiDevice::Meta_Timesig) {
        char lbBeat = 0;
        for (; lbBeat < 5; lbBeat++) { // natural logarithm, smallest is 128th
            if (1 << lbBeat >= b)
                break;
        }
        tc.append(writeTime(timeIncrement(time)));
        tc.append(static_cast<char>(CAMidiDevice::Midi_Ctl_Event));
        tc.append(event);
        tc.append(variableLengthValue(4));
        tc.append(a);
        tc.append(lbBeat);
        tc.append(18);
        tc.append(8);
        trackChunk.append(tc);
    } else if (event == CAMidiDevice::Meta_Tempo) {
        int usPerQuarter = 60000000 / a;
        tc.append(writeTime(timeIncrement(time)));
        tc.append(static_cast<char>(CAMidiDevice::Midi_Ctl_Event));
        tc.append(event);
        tc.append(variableLengthValue(3));
        char q;
        q = char(usPerQuarter >> 16);
        tc.append(q);
        q = char(usPerQuarter >> 8);
        tc.append(q);
        q = char(usPerQuarter >> 0);
        tc.append(q);
        trackChunk.append(tc);
    }
}

/// \todo: replace with enum midiCommands
#define META_TEXT 0x01
#define META_TIMESIG 0x58
#define META_KEYSIG 0x59
#define META_TEMPO 0x51
#define META_TRACK_END 0x2f

#define MIDI_CTL_EVENT 0xff
#define MIDI_CTL_REVERB 0x5b
#define MIDI_CTL_CHORUS 0x5d
#define MIDI_CTL_PAN 0x0a
#define MIDI_CTL_VOLUME 0x07
#define MIDI_CTL_SUSTAIN 0x40

/*!
    Converts 16-bit number to big endian byte array.
*/
QByteArray CAMidiExport::word16(short x)
{
    QByteArray ba;
    ba.append(x >> 8);
    ba.append(x);
    return ba;
}

QByteArray CAMidiExport::variableLengthValue(int value)
{

    QByteArray chunk;
    char b;
    bool byteswritten = false;
    b = (value >> 3 * 7) & 0x7f;
    if (b) {
        chunk.append(0x80 | b);
        byteswritten = true;
    }
    b = (value >> 2 * 7) & 0x7f;
    if (b || byteswritten) {
        chunk.append(0x80 | b);
        byteswritten = true;
    }
    b = (value >> 7) & 0x7f;
    if (b || byteswritten) {
        chunk.append(0x80 | b);
        byteswritten = true;
    }
    b = value & 0x7f;
    chunk.append(b);
    return chunk;
}

QByteArray CAMidiExport::writeTime(int time)
{
    char b;
    bool byteswritten = false;
    QByteArray ba;

    b = (time >> 3 * 7) & 0x7f;
    if (b) {
        ba.append(0x80 | b);
        byteswritten = true;
    }
    b = (time >> 2 * 7) & 0x7f;
    if (b || byteswritten) {
        ba.append(0x80 | b);
        byteswritten = true;
    }
    b = (time >> 7) & 0x7f;
    if (b || byteswritten) {
        ba.append(0x80 | b);
        byteswritten = true;
    }
    b = time & 0x7f;
    ba.append(b);
    return ba;
}

QByteArray CAMidiExport::trackEnd(void)
{
    QByteArray tc;
    tc.append(writeTime(0));
    /// \todo replace with enum
    tc.append(static_cast<char>(MIDI_CTL_EVENT));
    tc.append(META_TRACK_END);
    tc.append(static_cast<char>(0));
    return tc;
}

QByteArray CAMidiExport::textEvent(int time, QString s)
{
    QByteArray tc;
    tc.append(writeTime(time));
    /// \todo replace with enum
    tc.append(static_cast<char>(MIDI_CTL_EVENT));
    tc.append(META_TEXT);
    tc.append(variableLengthValue(s.length()));
    tc.append(s);
    return tc;
}

/*!
	Exports the current document to Lilypond syntax as a complete .ly file.
*/
void CAMidiExport::exportDocumentImpl(CADocument* doc)
{
    if (doc->sheetList().size() < 1) {
        //TODO: no sheets, raise an error
        return;
    }

    // In the header chunk we need to know the count of tracks.
    // We export every non empty voice as separate track.
    // For now we export only the first sheet.
    CASheet* sheet = doc->sheetList()[0];
    setCurSheet(sheet);
    trackChunk.clear();

    // Let's playback this sheet and dump that into a file,
    // and for this we have our own midi driver.
    CAPlayback* _playback = new CAPlayback(sheet, this);
    _playback->run();

    int count = 0;
    for (int c = 0; c < doc->sheetList()[0]->contextList().size(); ++c) {
        switch (sheet->contextList()[c]->contextType()) {
        case CAContext::Staff: {
            // exportStaffVoices( static_cast<CAStaff*>(sheet->contextList()[c]) );
            CAStaff* staff = static_cast<CAStaff*>(sheet->contextList()[c]);
            for (int v = 0; v < staff->voiceList().size(); ++v) {
                setCurVoice(staff->voiceList()[v]);
                count++;
                //std::cout << "Hallo  " << c << " " << v << "\n" << std::endl;
            }
            break;
        }
        case CAContext::ChordNameContext:
            break; // TODO
        case CAContext::LyricsContext:
        case CAContext::FunctionMarkContext:
        case CAContext::FiguredBassContext:
            break;
        }
    }

    writeFile();
}

/*!
	Exports the current document to Lilypond syntax as a complete .ly file.
*/
void CAMidiExport::exportSheetImpl(CASheet* sheet)
{
    /*
	if ( doc->sheetList().size() < 1 ) {
		//TODO: no sheets, raise an error
		return;
	}


	// In the header chunk we need to know the count of tracks.
	// We export every non empty voice as separate track.
	// For now we export only the first sheet.
	CASheet *sheet = doc->sheetList()[ 0 ];
*/
    setCurSheet(sheet);
    trackChunk.clear();

    // Let's playback this sheet and dump that into a file,
    // and for this we have our own midi driver.
    CAPlayback* _playback = new CAPlayback(sheet, this);
    _playback->run();

    int count = 0;
    for (int c = 0; c < sheet->contextList().size(); ++c) {
        switch (sheet->contextList()[c]->contextType()) {
        case CAContext::Staff: {
            // exportStaffVoices( static_cast<CAStaff*>(sheet->contextList()[c]) );
            CAStaff* staff = static_cast<CAStaff*>(sheet->contextList()[c]);
            for (int v = 0; v < staff->voiceList().size(); ++v) {
                setCurVoice(staff->voiceList()[v]);
                count++;
                //std::cout << "Hallo  " << c << " " << v << "\n" << std::endl;
            }
            break;
        }
        case CAContext::LyricsContext:
        case CAContext::FunctionMarkContext:
        case CAContext::FiguredBassContext:
        case CAContext::ChordNameContext:
            break;
        }
    }

    writeFile();
}

void CAMidiExport::writeFile()
{

    QByteArray headerChunk;
    headerChunk.append("MThd...."); // header and space for length
    headerChunk.append(word16(1)); // Midi-Format version
    headerChunk.append(word16(2)); // number of tracks, a control track and a music track for a trying out ...
    /// \todo QByteArray does not support char values > 0x7f
    headerChunk.append(word16(static_cast<short>(CAPlayableLength::playableLengthToTimeLength(CAPlayableLength::Quarter)))); // time division ticks per quarter
    setChunkLength(&headerChunk);
    streamQByteArray(headerChunk);

    QByteArray controlTrackChunk;
    controlTrackChunk.append("MTrk....");
    controlTrackChunk.append(textEvent(0, QString("Canorus Version ") + CANORUS_VERSION + " generated. "));
    controlTrackChunk.append(textEvent(0, "It's still a work in progress."));
    controlTrackChunk.append(trackEnd());
    setChunkLength(&controlTrackChunk);
    streamQByteArray(controlTrackChunk);

    // trackChunk is already filled with midi data,
    // let's add chunk header, in reverse, ...
    trackChunk.prepend("MTrk....");
    // ... and add the tail:
    trackChunk.append(trackEnd());
    setChunkLength(&trackChunk);
    streamQByteArray(trackChunk);
}

void CAMidiExport::setChunkLength(QByteArray* x)
{
    /// \todo QByteArray does not support char values > 0x7f
    qint32 l = (*x).size() - 8; // subtract header length
    for (int i = 0; i < 4; i++) {
        (*x)[7 - i] = static_cast<char>((l >> (8 * i)));
    }
}

void CAMidiExport::streamQByteArray(QByteArray x)
{
    for (int i = 0; i < x.size(); i++) // here we pass binary data through QTextStream
        out().device()->putChar(x[i]);

#ifdef QT_DEBUG
    for (int i = 0; i < x.size(); i++) {
        printf(" %02x", 0x0ff & x.at(i));
    }
    printf("\n");
#endif

    return; // when no debugging
}
