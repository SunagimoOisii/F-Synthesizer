#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <filesystem>

#include "AudioBuffer.h"
#include "MIDIParser.h"
#include "Sequencer.h"
#include "SynthEngine/SynthEngine.h"
#include "Writer.h"

int main()
{
    //出力バッファ
    SoundData sound(6 * 44100, 16, 44100);

    //MIDI入力設定
    const std::string midiPath = (std::filesystem::path("assets") / "midi" / "logo.mid").string();
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
    auto makeNoise = [](NoiseType noise,
        double amp, double atk, double dec, double sus, double rel)
    {
        return ChannelConfig{ NoiseConfig{ noise }, amp, atk, dec, sus, rel };
    };
    auto makeFm = [](WaveType carrierWave, WaveType modWave,
        double amp, double atk, double dec, double sus, double rel,
        double carrierRatio, double modRatio, double index, double outLevel)
    {
        return ChannelConfig{ FmConfig{ carrierWave, modWave, carrierRatio, modRatio, index, outLevel },
            amp, atk, dec, sus, rel };
    };
    auto makeWave = [&](WaveType wave,
        double amp, double atk, double dec, double sus, double rel)
    {
        return makeFm(wave, wave, amp, atk, dec, sus, rel, 1.0, 1.0, 0.0, 1.0);
    };
    auto makeDrum = [](DrumType type,
        double amp, double atk, double dec, double sus, double rel)
    {
        return ChannelConfig{ DrumConfig{ type }, amp, atk, dec, sus, rel };
    };
    auto makeDrumDetail = [](const DrumConfig& drum,
        double amp, double atk, double dec, double sus, double rel)
    {
        return ChannelConfig{ drum, amp, atk, dec, sus, rel };
    };
    auto makeDrumKitDetail = [](const DrumKitConfig& kit,
        double amp, double atk, double dec, double sus, double rel)
    {
        return ChannelConfig{ kit, amp, atk, dec, sus, rel };
    };
    auto makeGmDrumKit = []()
    {
        DrumKitConfig kit{};
        for (auto& d : kit.map)
        {
            d.type = DrumType::None;
        }

        DrumConfig kick{ DrumType::Kick };
        kick.gain = 0.6;
        kick.baseFreq = 60.0;
        kick.pitchDrop = 3.0;
        kick.pitchDecaySec = 0.06;

        DrumConfig snare{ DrumType::Snare };
        snare.gain = 0.6;
        snare.toneFreq = 220.0;
        snare.toneLevel = 0.55;
        snare.noiseLevel = 0.35;
        snare.hpCut = 700.0;
        snare.lpCut = 6000.0;
        snare.toneWave = (int)WaveType::Triangle;
        snare.noiseType = (int)NoiseType::White;

        DrumConfig hat{ DrumType::Hat };
        hat.gain = 0.15;
        hat.toneFreq = 8000.0;
        hat.toneLevel = 0.2;
        hat.noiseLevel = 0.2;
        hat.hpCut = 4000.0;
        hat.lpCut = 6000.0;
        hat.toneWave = (int)WaveType::Sine;
        hat.noiseType = (int)NoiseType::White;

        kit.map[36] = kick; // Bass Drum 1
        kit.map[38] = snare; // Acoustic Snare
        kit.map[40] = snare; // Electric Snare
        kit.map[42] = hat; // Closed Hi-Hat
        kit.map[44] = hat; // Pedal Hi-Hat
        kit.map[46] = hat; // Open Hi-Hat
        kit.map[49] = hat; // Crash Cymbal 1

        return kit;
    };

    //基本
    //amp:全体音量
    //atk:立ち上がりの柔らかさ, 硬さ
    //dec:余韻の長さ, 短さ
    //sus:太く長く, 短く細く
    //rel:音が消えるまで長く, 短く

    //FM
    //carrierWave:音の基本キャラ(Sine = 丸い、Triangle = 柔らかい、Saw, Square = 明るく硬い)
    //modWave:倍音の質感(Sine, Trl = 滑らか、Saw, Square = 荒い, 硬い)
    //carrierRatio:基音に対するピッチ感(整数比 = 安定, 非整数 = 濁り, 不協和)
    //modRatio:倍音が高域寄り(明るく金属的), 低域寄り(やわらかい)
    //index:倍音量が増える(明るく金属的), 減るか(やわらかく素直)
    //outLevel:FM成分の存在感がある(抜ける), ないか(大人しい)

    //DrumConfig
    //gain:ampみたいなもの
    //Kick:baseFreq(基音:低いほど重い), pitchDrop(落ち幅:大きいほどドン感), pitchDecaySec(落ち速度:短いほどタイト)
    //Snare:toneFreq(高いほど硬い, 軽い), toneLevel(上げると音程感が増える),
    //       noiseLevel(上げるとザラつき), hpCut(上げると低域が削れる), lpCut(下げるとこもる), toneWave/NoiseType(質感)
    //Hat:toneFreq(高いほどシャリッ), toneLevel(上げるとトーン強),
    //     noiseLevel(上げると粒立ち), hpCut(上げると薄く, 軽く), lpCut(下げるとこもる), toneWave/NoiseType(質感)

    //Beginning
    /*
    std::array<ChannelConfig, 16> channelConfigs = {
        //オルガン
        makeFm(WaveType::Sine, WaveType::Triangle, 0.4, 0.005, 0.08, 0.65, 0.15, 1.0, 1.01, 2.5, 1.0), // ch0
        makeFm(WaveType::Sine, WaveType::Sine, 0.45, 0.005, 0.08, 0.65, 0.15, 1.0, 1.005, 3.5, 1.0), // ch1
        makeFm(WaveType::Sine, WaveType::Triangle, 0.4, 0.005, 0.08, 0.65, 0.15, 1.0, 1.01, 2.5, 1.0), // ch2
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch3
        makeFm(WaveType::Sine, WaveType::Triangle, 0.4, 0.005, 0.08, 0.65, 0.15, 1.0, 1.02, 2.0, 1.0), // ch4
        //ベース
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.4, 0.01, 0.10, 0.60, 0.20, 0.5, 0.995, 2.0, 1.0), // ch5
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.4, 0.01, 0.10, 0.60, 0.20, 0.5, 0.995, 2.5, 1.0), // ch6
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch7
        makeWave(WaveType::Square, 0.5, 0.01, 0.1, 0.8, 0.2), // ch8
        //ドラム
        makeDrumKitDetail(makeGmDrumKit(), 15.0, 0.001, 0.15, 0.1, 0.3), //ch9
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch10
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch11
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch12
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch13
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch14
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2)  // ch15
    };*/

    //カエルのテーマ（MIDI: ch0/1=フルート, ch2/3=トランペット, ch4/5=ストリングス, ch6/7=ベース, ch9=ドラム）
    std::array<ChannelConfig, 16> channelConfigs = {
        //フルート
        makeFm(WaveType::Sine, WaveType::Triangle, 0.40, 0.04, 0.22, 0.90, 0.35, 1.0, 2.01, 1.45, 0.9), // ch0
        makeFm(WaveType::Sine, WaveType::Triangle, 0.38, 0.04, 0.22, 0.90, 0.35, 1.0, 2.015, 1.45, 0.9), // ch1
        //トランペット
        makeFm(WaveType::Saw, WaveType::Triangle, 0.45, 0.0025, 0.18, 0.68, 0.18, 1.0, 0.9965, 1.3, 1.15), // ch2
        makeFm(WaveType::Saw, WaveType::Square, 0.48, 0.0025, 0.18, 0.68, 0.18, 1.0, 0.9965, 1.3, 1.15), // ch3
        //ストリングス
        makeFm(WaveType::Sine, WaveType::Triangle, 0.30, 0.08, 0.40, 0.80, 0.55, 1.0, 1.01, 1.4, 1.0), // ch4
        makeFm(WaveType::Triangle, WaveType::Sine, 0.26, 0.06, 0.32, 0.72, 0.45, 1.0, 1.01, 1.4, 1.0), // ch5
        //ベース
        makeFm(WaveType::Triangle, WaveType::Sine, 0.5, 0.006, 0.14, 0.62, 0.2, 0.5, 0.9975, 1.0, 1.05), // ch6
        makeFm(WaveType::Triangle, WaveType::Sine, 0.47, 0.006, 0.14, 0.62, 0.2, 0.5, 0.9975, 1.0, 1.05), // ch7
        makeFm(WaveType::Square, WaveType::Square, 0.22, 0.01, 0.1, 0.75, 0.2, 1.0, 1.0, 0.9975, 1.0), // ch8
        //ドラム
        makeDrumKitDetail(makeGmDrumKit(), 10.0, 0.001, 0.15, 0.1, 0.3), //ch9
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.25, 0.02, 0.1, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0), // ch10
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.25, 0.02, 0.1, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0), // ch11
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.25, 0.02, 0.1, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0), // ch12
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.25, 0.02, 0.1, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0), // ch13
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.25, 0.02, 0.1, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0), // ch14
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.25, 0.02, 0.1, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0)  // ch15
    };

    //乾坤の血族
    /*
    std::array<ChannelConfig, 16> channelConfigs = {
        //リード
        makeFm(WaveType::Triangle, WaveType::Saw, 0.5, 0.005, 0.1, 0.7, 0.08, 1.0, 1.0, 2.5, 1.0), // ch0
        //サブ
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.3, 0.0015, 0.2, 0.85, 0.3, 1.01, 0.9975, 7.0, 1.0), // ch1
        //ベース
        makeFm(WaveType::Triangle, WaveType::Sine, 0.55, 0.002, 0.12, 0.65, 0.06, 1.03, 1.0, 1.5, 1.0), // ch2
        //ストリングス1,2
        makeFm(WaveType::Sine, WaveType::Triangle, 0.3, 0.06, 0.35, 0.78, 0.45, 1.0, 1.01, 1.75, 1.0), // ch3
        makeFm(WaveType::Triangle, WaveType::Sine, 0.25, 0.03, 0.25, 0.65, 0.3, 1.0, 1.01, 1.75, 1.0), // ch4
        //コードギター
        makeFm(WaveType::Triangle, WaveType::Square, 0.32, 0.003, 0.22, 0.3, 0.1, 1.0, 3.0, 1.4, 1.0), // ch5
        //歪みギター1,2,3
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.3, 0.001, 0.22, 0.35, 0.1, 1.0, 3.0, 1.6, 1.0), // ch6
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.38, 0.002, 0.1, 0.6, 0.08, 1.0, 2.0, 1.3, 1.0), // ch7
        //サブ
        makeFm(WaveType::Sine, WaveType::Triangle, 0.4, 0.005, 0.1, 0.7, 0.08, 1.0, 1.0, 2.75, 1.0), // ch8
        //ドラム
        makeDrumKitDetail(makeGmDrumKit(), 4.0, 0.001, 0.15, 0.1, 0.2), //ch9
        makeFm(WaveType::Sine, WaveType::Sine, 0.5, 0.25, 0.35, 0.6, 0.8, 1.0, 1.0, 0.3, 1.0), // ch10
        makeFm(WaveType::Sine, WaveType::Sine, 0.45, 0.25, 0.25, 0.6, 0.8, 1.0, 1.0, 0.3, 1.0), // ch11
        makeFm(WaveType::Sine, WaveType::Sine, 0.45, 0.25, 0.25, 0.6, 0.8, 1.0, 1.0, 0.3, 1.0), // ch12
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch13
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch14
        makeFm(WaveType::Sine, WaveType::Sine, 0.45, 0.25, 0.25, 0.6, 0.8, 1.0, 1.025, 0.3, 1.0), // ch15
    };*/

    //BloodyTears
    /*
    std::array<ChannelConfig, 16> channelConfigs = {
        //主旋律：FM / 金属感・不安定さ（目立たせる）
        makeFm(WaveType::Triangle, WaveType::Saw, 0.35, 0.005, 0.08, 0.65, 0.15, 1.0, 1.015, 4.5, 1.0), // ch0
        //副旋律：FM / 安定した輪郭（主旋律を邪魔しない）
        makeFm(WaveType::Sine, WaveType::Sine, 0.35, 0.015, 0.12, 0.50, 0.25, 1.0, 1.02, 2.5, 1.0), // ch1
        //ベース：FM / 軽いジャギり・低域の汚れ
        makeFm(WaveType::Saw, WaveType::Square, 0.3, 0.01, 0.10, 0.60, 0.20, 0.5, 1.02, 1.5, 1.0), // ch2
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch3
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch4
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch5
        makeWave(WaveType::Square, 0.5, 0.05, 0.15, 0.7, 0.2), // ch6
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch7
        makeWave(WaveType::Square, 0.5, 0.01, 0.1, 0.8, 0.2), // ch8
        //ノイズ：PSG / リズム・不穏さの付与
        makeNoise(NoiseType::Blue, 0.25, 0.001, 0.03, 0.15, 0.05), // ch9
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch10
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch11
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch12
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch13
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch14
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2)  // ch15
    };
    */

    //Solstice
    /*
    std::array<ChannelConfig, 16> channelConfigs = {
        //主旋律：FM / 金属感・不安定さ（目立たせる）
        makeFm(WaveType::Sine, WaveType::Triangle, 0.45, 0.005, 0.08, 0.65, 0.15, 1.0, 3.0, 1.5, 1.0), // ch0
        //副旋律：PSG / 安定した輪郭（主旋律を邪魔しない）
        makeFm(WaveType::Triangle, WaveType::Saw, 0.4, 0.01, 0.10, 0.60, 0.20, 0.5, 2.0, 0.8, 1.0), // ch1
        //ベース：FM / 軽いジャギり・低域の汚れ
        makeFm(WaveType::Sine, WaveType::Saw, 0.4, 0.01, 0.10, 0.60, 0.20, 0.5, 1.0, 0.5, 1.0), // ch2
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch3
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch4
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch5
        makeWave(WaveType::Square, 0.5, 0.05, 0.15, 0.7, 0.2), // ch6
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch7
        makeWave(WaveType::Square, 0.5, 0.01, 0.1, 0.8, 0.2), // ch8
        //ノイズ：PSG / リズム・不穏さの付与
        //makeNoise(NoiseType::Blue, 0.45, 0.001, 0.03, 0.15, 0.05), // ch9
        makeFm(WaveType::Saw, WaveType::Square, 0.35, 0.001, 0.03, 0.15, 0.05, 0.5, 1.0, 0.5, 1.0),
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch10
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch11
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch12
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch13
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2), // ch14
        makeWave(WaveType::Triangle, 0.5, 0.05, 0.15, 0.7, 0.2)  // ch15
    };*/
    
    for (int i = 0;i < channelConfigs.size();i++)
     {
         //if (i != 1) channelConfigs.at(i).amp = 0;
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
