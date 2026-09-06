#pragma once

// PSG波形種別。
enum class PsgWaveType
{
    Square,    // デューティ固定 50%
    Pulse,     // デューティ可変（duty パラメータで制御）
    Triangle,
    Noise      // 16bit LFSR 白ノイズ
};

// PSG発振方式の設定集合。
// チップ制約（離散ボリューム・デューティサイクル・ボイス数上限）を再現する。
// smoothing は非対応（契約上 waveform 専用）。
struct PsgConfig
{
    bool operator==(const PsgConfig&) const = default;
    PsgWaveType wave = PsgWaveType::Square;
    // パルス幅（0-7, 1/8刻み。0=12.5%, 4=50%, 7=87.5%）。wave=Pulse のみ有効。
    int duty = 4;
    // 4bit音量量子化ステップ（0-15。15=最大、0=無音）。
    int volumeSteps = 15;
    // ポリフォニー上限（1-8。チップ制約の再現用）。
    int maxVoices = 3;
};
