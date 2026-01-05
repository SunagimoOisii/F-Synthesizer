#include <iostream>
#include <string>
#include <array>
#include <vector>

#include "AudioBuffer.h"
#include "MIDIParser.h"
#include "Sequencer.h"
#include "SynthEngine.h"
#include "Writer.h"

int main()
{
    //出力バッファ
    SoundData sound(6 * 44100, 16, 44100);

    //MIDI入力設定
    const std::string midiPath = "test.mid";
    const int targetChannel = -1; // -1で全チャネル
    const WaveType defaultWave = WaveType::Saw;

    //MIDI読み込み
    std::vector<MIDIEventTick> ticks;
    std::vector<TempoEvent> tempoEvents;
    int midiTPQ = 0;
    MIDIParseStatus stats{};
    if (!LoadMIDIBasic(midiPath, targetChannel, ticks, tempoEvents, midiTPQ, stats))
    {
        std::cout << "Failed to load MIDI: " << midiPath << std::endl;
        return 1;
    }

    //MIDIイベント統計
    int noteCount = 0;
    int ccCount = 0;
    std::array<int, 16> noteByChannel{};
    std::array<int, 16> ccByChannel{};
    std::array<int, 128> ccByType{};
    for (int i = 0; i < 16; i++)
    {
        noteByChannel[i] = 0;
        ccByChannel[i] = 0;
    }
    for (int i = 0; i < 128; i++)
    {
        ccByType[i] = 0;
    }
    for (const auto& t : ticks)
    {
        int ch = (t.channel >= 0 && t.channel < 16) ? t.channel : 0;
        if (t.type == MIDIEventType::Note)
        {
            noteCount++;
            noteByChannel[ch]++;
        }
        else if (t.type == MIDIEventType::ControlChange)
        {
            ccCount++;
            ccByChannel[ch]++;
            int c = t.controller;
            if (c < 0) c = 0;
            if (c > 127) c = 127;
            ccByType[c]++;
        }
    }

    //読み込み結果の表示
    std::cout << "MIDI Info: format=" << stats.format
        << ", tracks=" << stats.numTracks
        << ", TPQ=" << midiTPQ
        << ", tempoEvents=" << tempoEvents.size() << std::endl;
    std::cout << "Event Counts: note=" << noteCount
        << ", cc=" << ccCount
        << ", tempo=" << tempoEvents.size() << std::endl;
    std::cout << "Channel Note Counts:";
    for (int ch = 0; ch < 16; ch++)
    {
        std::cout << " ch" << ch << "=" << noteByChannel[ch];
    }
    std::cout << std::endl;
    std::cout << "Channel CC Counts:";
    for (int ch = 0; ch < 16; ch++)
    {
        std::cout << " ch" << ch << "=" << ccByChannel[ch];
    }
    std::cout << std::endl;
    std::cout << "CC Types:";
    for (int c = 0; c < 128; c++)
    {
        if (ccByType[c] > 0)
        {
            std::cout << " cc" << c << "=" << ccByType[c];
        }
    }
    std::cout << std::endl;
    std::cout << "Unsupported Events: " << stats.unsupportedEvents << std::endl;

    //tick -> sample のイベント変換
    std::vector<MidiEvent> events;
    BuildSampleEvents(ticks, tempoEvents, midiTPQ, sound.fs, defaultWave, events);

    if (events.empty())
    {
        std::cout << "No note events found." << std::endl;
        return 1;
    }

    //チャンネル別プリセット
     std::array<ChannelConfig, 16> channelConfigs = {
        //mode, type, amp, atk, dec, sus, rel, cRatio, mRatio, mIndex, out
        ChannelConfig{ SynthMode::FM, WaveType::Square, 0.5, 0.01, 0.1, 0.8, 0.2, 1.0, 1.0, 4.0, 1.0 },         // ch0
        ChannelConfig{ SynthMode::Basic, WaveType::Triangle, 0.4, 0.02, 0.2, 0.6, 0.3, 1.0, 1.5, 1.0, 1.0 },      // ch1
        ChannelConfig{ SynthMode::Basic, WaveType::Saw, 0.45, 0.05, 0.15, 0.7, 0.25, 1.0, 1.0, 1.0, 1.0 }, // ch2
        ChannelConfig{ SynthMode::Basic, WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0 },       // ch3
        ChannelConfig{ SynthMode::Basic, WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0 },       // ch4
        ChannelConfig{ SynthMode::Basic, WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2, 1.0, 1.5, 1.0, 1.0 },       // ch5
        ChannelConfig{ SynthMode::Basic, WaveType::Square, 0.5, 0.05, 0.15, 0.7, 0.2, 1.0, 1.5, 1.0, 1.0 },       // ch6
        ChannelConfig{ SynthMode::Basic, WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0 },       // ch7
        ChannelConfig{ SynthMode::FM, WaveType::Square, 0.5, 0.01, 0.1, 0.8, 0.2, 1.0, 1.0, 4.0, 1.0 },       // ch8
        ChannelConfig{ SynthMode::Basic, WaveType::Noise, 0.6, 0.001, 0.05, 0.2, 0.05, 1.0, 1.0, 0.0, 1.0 },    // ch9
        ChannelConfig{ SynthMode::Basic, WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0 },       // ch10
        ChannelConfig{ SynthMode::Basic, WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0 },       // ch11
        ChannelConfig{ SynthMode::Basic, WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0 },       // ch12
        ChannelConfig{ SynthMode::Basic, WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0 },       // ch13
        ChannelConfig{ SynthMode::Basic, WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0 },       // ch14
        ChannelConfig{ SynthMode::Basic, WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0 }        // ch15
    };

    //バッファ長の調整
    int lastSample = events.back().sample;
    int extraRelease = (int)(0.3 * sound.fs);
    int neededSamples = lastSample + extraRelease + 1;
    if (neededSamples > sound.length)
    {
        sound = SoundData(neededSamples, sound.bits, sound.fs);
    }

    std::cout << "Events: " << events.size()
        << ", FirstSample: " << events.front().sample
        << ", LastSample: " << events.back().sample
        << ", Length: " << sound.length << std::endl;

    //合成処理
    RenderMidiEvents(sound, events, channelConfigs);

    //書き出し
    SaveWavFile(sound, "test_notes.wav");
    std::cout << "Saved SoundData" << std::endl;

    return 0;
}
