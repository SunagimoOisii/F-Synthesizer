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
    const std::string midiPath = "test3.mid";
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
    std::vector<MIDIEvent> events;
    BuildSampleEvents(ticks, tempoEvents, midiTPQ, sound.fs, defaultWave, events);

    if (events.empty())
    {
        std::cout << "No note events found." << std::endl;
        return 1;
    }

    //チャンネル別プリセット
    
    //Beginning
    /*
    std::array<ChannelConfig, 16> channelConfigs = {
        //mode, source, type, noise, amp, atk, dec, sus, rel, carrierWave, cRatio, mRatio, mIndex, out
        //オルガン
        ChannelConfig{ SynthMode::FM,    SourceType::Waveform, WaveType::Sine,   NoiseType::White, 0.4, 0.005, 0.08, 0.65, 0.15, WaveType::Triangle, 1.0,   1.015,  4.5,   1.0 }, // ch0
        ChannelConfig{ SynthMode::FM,    SourceType::Waveform, WaveType::Sine,   NoiseType::White, 0.45, 0.005, 0.08, 0.65, 0.15, WaveType::Triangle, 1.0,   1.015,  4.5,   1.0 }, // ch1
        ChannelConfig{ SynthMode::FM,    SourceType::Waveform, WaveType::Sine,   NoiseType::White, 0.4, 0.005, 0.08, 0.65, 0.15, WaveType::Triangle, 1.0,   1.015,  4.5,   1.0 }, // ch2
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch3
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch4
        //ベース
        ChannelConfig{ SynthMode::FM,    SourceType::Waveform, WaveType::Square,   NoiseType::White, 0.4,  0.01,  0.10, 0.60, 0.20, WaveType::Sine, 0.5,   1.03,  2.75,   1.0 },       // ch5
        ChannelConfig{ SynthMode::FM,    SourceType::Waveform, WaveType::Square,   NoiseType::White, 0.4,  0.01,  0.10, 0.60, 0.20, WaveType::Sine, 0.5,   1.03,  2.75,   1.0 },       // ch6
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch7
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Square,   NoiseType::White, 0.5,  0.01, 0.1,  0.8,  0.2, WaveType::Sine,  1.0, 1.0,  4.0, 1.0 },       // ch8
        //ノイズ
        ChannelConfig{ SynthMode::Basic, SourceType::Noise,    WaveType::Sine,     NoiseType::Blue, 0.45,  0.001, 0.03, 0.15, 0.05, WaveType::Sine, 1.0,  1.0,  0.0, 1.0 }, // ch9
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch10
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch11
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch12
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch13
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch14
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 }        // ch15
    };
    */

    //BloodyTears
    std::array<ChannelConfig, 16> channelConfigs = {
        //mode, source, type, noise, amp, atk, dec, sus, rel, carrierWave, cRatio, mRatio, mIndex, out
        //主旋律：FM / 金属感・不安定さ（目立たせる）
        ChannelConfig{ SynthMode::FM,    SourceType::Waveform, WaveType::Square,   NoiseType::White, 0.35, 0.005, 0.08, 0.65, 0.15, WaveType::Triangle, 1.0,   1.015,  4.5,   1.0 }, // ch0
        //副旋律：PSG / 安定した輪郭（主旋律を邪魔しない）
        ChannelConfig{ SynthMode::FM, SourceType::Waveform, WaveType::Saw,      NoiseType::White, 0.35, 0.015, 0.12, 0.50, 0.25, WaveType::Sine, 1.0,   1.02,   2.5,   1.0 }, // ch1
        //ベース：FM / 軽いジャギり・低域の汚れ
        ChannelConfig{ SynthMode::FM,    SourceType::Waveform, WaveType::Square,   NoiseType::White, 0.3,  0.01,  0.10, 0.60, 0.20, WaveType::Saw, 0.5,   1.02,  1.5,   1.0 }, // ch2
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch3
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch4
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.5,  1.0, 1.0 },       // ch5
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Square,   NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.5,  1.0, 1.0 },       // ch6
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch7
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Square,   NoiseType::White, 0.5,  0.01, 0.1,  0.8,  0.2, WaveType::Sine,  1.0, 1.0,  4.0, 1.0 },       // ch8
        //ノイズ：PSG / リズム・不穏さの付与
        ChannelConfig{ SynthMode::Basic, SourceType::Noise,    WaveType::Sine,     NoiseType::Blue, 0.25,  0.001, 0.03, 0.15, 0.05, WaveType::Sine, 1.0,  1.0,  0.0, 1.0 }, // ch9
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch10
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch11
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch12
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch13
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch14
        ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 }        // ch15
    };

//Solstice
    /*
 std::array<ChannelConfig, 16> channelConfigs = {
    //mode, source, type, noise, amp, atk, dec, sus, rel, carrierWave, cRatio, mRatio, mIndex, out
    //主旋律：FM / 金属感・不安定さ（目立たせる）
    ChannelConfig{ SynthMode::FM,    SourceType::Waveform, WaveType::Square,   NoiseType::White, 0.45, 0.005, 0.08, 0.65, 0.15, WaveType::Triangle, 1.0,   1.015,  7.5,   1.0 }, // ch0
    //副旋律：PSG / 安定した輪郭（主旋律を邪魔しない）
    ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Saw,      NoiseType::White, 0.30, 0.015, 0.12, 0.50, 0.25, WaveType::Sine, 1.0,   1.0,   0.0,   1.0 }, // ch1
    //ベース：FM / 軽いジャギり・低域の汚れ
    ChannelConfig{ SynthMode::FM,    SourceType::Waveform, WaveType::Square,   NoiseType::White, 0.4,  0.01,  0.10, 0.60, 0.20, WaveType::Triangle, 0.5,   1.0,  1.5,   1.0 }, // ch2
    ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch3
    ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch4
    ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.5,  1.0, 1.0 },       // ch5
    ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Square,   NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.5,  1.0, 1.0 },       // ch6
    ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch7
    ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Square,   NoiseType::White, 0.5,  0.01, 0.1,  0.8,  0.2, WaveType::Sine,  1.0, 1.0,  4.0, 1.0 },       // ch8
    //ノイズ：PSG / リズム・不穏さの付与
    ChannelConfig{ SynthMode::Basic, SourceType::Noise,    WaveType::Sine,     NoiseType::Blue, 0.45,  0.001, 0.03, 0.15, 0.05, WaveType::Sine, 1.0,  1.0,  0.0, 1.0 }, // ch9
    ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch10
    ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch11
    ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch12
    ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch13
    ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 },       // ch14
    ChannelConfig{ SynthMode::Basic, SourceType::Waveform, WaveType::Triangle, NoiseType::White, 0.5,  0.05, 0.15, 0.7,  0.2, WaveType::Sine,  1.0, 1.0,  0.0, 1.0 }        // ch15
};*/
     for (int i = 0;i < channelConfigs.size();i++)
     {
         //if (i != 9) channelConfigs.at(i).amp = 0;
     }

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
    RenderMIDIEvents(sound, events, channelConfigs);

    //書き出し
    auto wavTitle = "test_notes.wav";
    SaveWavFile(sound, wavTitle);
    std::cout << "Saved SoundData: " << wavTitle << std::endl;

    return 0;
}


