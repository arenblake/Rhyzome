#include "daisy_field.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyField                           hw;
SyntheticBassDrum                    bd;
SyntheticSnareDrum                   sd;
HiHat<SquareNoise, LinearVCA, false> hh;
HiHat<RingModNoise, LinearVCA, true> cymbal;
Metro                                tick;

Compressor comp;
Overdrive  drive;
Limiter    limiter;

int step {0};
bool t;
bool locked {false};

const int MENU_COUNT {5};

struct DrumState {
	bool seq[16] {false};
	float kvals[8] {0.0};
	bool isLatched[8] {false};
};

DrumState drumStates[MENU_COUNT];

int selectedMenu {0};

size_t keyboard_leds[] = {
	DaisyField::LED_KEY_B1,
	DaisyField::LED_KEY_B2,
	DaisyField::LED_KEY_B3,
	DaisyField::LED_KEY_B4,
	DaisyField::LED_KEY_B5,
	DaisyField::LED_KEY_B6,
	DaisyField::LED_KEY_B7,
	DaisyField::LED_KEY_B8,
	DaisyField::LED_KEY_A1,
	DaisyField::LED_KEY_A2,
	DaisyField::LED_KEY_A3,
	DaisyField::LED_KEY_A4,
	DaisyField::LED_KEY_A5,
	DaisyField::LED_KEY_A6,
	DaisyField::LED_KEY_A7,
	DaisyField::LED_KEY_A8,
};
	
void handleButton() {
	for(size_t i = 0; i < 16; i++)
    {
		if (hw.KeyboardRisingEdge(i)) drumStates[selectedMenu].seq[(i + 8) % 16] = !drumStates[selectedMenu].seq[(i + 8) % 16];
    }
}

void updateLeds() {
	for(size_t i = 0; i < 16; i++)
    {
		if (drumStates[selectedMenu].seq[i] == true) hw.led_driver.SetLed(keyboard_leds[i], 1.f);
		if (drumStates[selectedMenu].seq[i] != true) hw.led_driver.SetLed(keyboard_leds[i], 0.f);
		hw.led_driver.SetLed(keyboard_leds[step], 0.65f);
    }

	hw.led_driver.SwapBuffersAndTransmit();
}

void changeMenu() {
	float prevMenu = selectedMenu;
	if(hw.sw[0].RisingEdge()) selectedMenu = selectedMenu == 0 ? MENU_COUNT - 2 : (selectedMenu - 1) % (MENU_COUNT - 1);
	if(hw.sw[1].RisingEdge()) selectedMenu = (selectedMenu + 1) % (MENU_COUNT - 1);
	if(hw.sw[0].Pressed() && hw.sw[1].Pressed() && !locked) {
		locked = true;
		if(selectedMenu == MENU_COUNT - 1) {
			selectedMenu = 0;
		} else {
			selectedMenu = MENU_COUNT - 1;
		}
	}
	if(hw.sw[0].FallingEdge() || hw.sw[1].FallingEdge()) locked = false;

	if(prevMenu != selectedMenu) {
		for(size_t i = 0; i < 8; i++) {
			drumStates[selectedMenu].isLatched[i] = false;
		}
	}
}

void buildString(const char lStrToWrt[15]) {
	char lStr[15];
	sprintf(lStr, lStrToWrt);
	hw.display.SetCursor(0, 0);
	hw.display.WriteString(lStr, Font_6x8, true);
}

void displayParamLabel(const char lStrToWrt[4], int curX, int curY) {
	char lStr[4];
	sprintf(lStr, lStrToWrt);
	hw.display.SetCursor(curX, curY);
	hw.display.WriteString(lStr, Font_6x8, true);
}

void displayParamValues(int knob, int curX, int curY) {
	char pStr[4];
	int pVal = int((drumStates[selectedMenu].kvals[knob]) * 100);
	snprintf(pStr, 4, "%d", pVal);
	hw.display.SetCursor(curX, curY);
	hw.display.WriteString(pStr, Font_7x10, true);
}

void displayMenu() {
	hw.display.DrawRect(0,12,hw.display.Width() - 1,hw.display.Height() - 1,true);
	hw.display.DrawLine(0,hw.display.Height() / 2 + 6, hw.display.Width(), hw.display.Height() / 2 + 6, true);
	hw.display.DrawLine(hw.display.Width() * 0.75, 12, hw.display.Width() * 0.75, hw.display.Height(), true);
	hw.display.DrawLine(hw.display.Width() / 2, 12, hw.display.Width() / 2, hw.display.Height(), true);
	hw.display.DrawLine(hw.display.Width() / 4, 12, hw.display.Width() / 4, hw.display.Height(), true);

	displayParamValues(0, 10, 27);
	displayParamValues(1, hw.display.Width() / 4 + 10, 27);
	displayParamValues(2, hw.display.Width() / 2 + 10, 27);
	displayParamValues(3, hw.display.Width() * 0.75 + 10, 27);
	displayParamValues(4, 10, 52);
	displayParamValues(5, hw.display.Width() / 4 + 10, 52);
	displayParamValues(6, hw.display.Width() / 2 + 10, 52);
	displayParamValues(7, hw.display.Width() * 0.75 + 10, 52);

	switch(selectedMenu) {
	case 0:
		buildString("Kick");
		displayParamLabel("freq", 4, 15);
		displayParamLabel("dky", hw.display.Width() / 4 + 8, 15);
		displayParamLabel("dirt", hw.display.Width() / 2 + 4, 15);
		displayParamLabel("tone", hw.display.Width() * 0.75 + 4, 15);
		displayParamLabel("acc", 7, 41);
		displayParamLabel("fmAmt", hw.display.Width() / 4 + 1, 41);
		displayParamLabel("fmDky", hw.display.Width() / 2 + 1, 41);

		displayParamLabel("Gain", hw.display.Width() * 0.75 + 4, 41);
		break;
	case 1:
		buildString("Snare");
		displayParamLabel("freq", 4, 15);
		displayParamLabel("dky", hw.display.Width() / 4 + 8, 15);
		displayParamLabel("acc", hw.display.Width() / 2 + 8, 15);
		displayParamLabel("snap", hw.display.Width() * 0.75 + 4, 15);
		displayParamLabel("fmAmt", 1, 41);

		displayParamLabel("Gain", hw.display.Width() * 0.75 + 4, 41);
		break;
	case 2:
		buildString("Hi-Hat");
		displayParamLabel("freq", 4, 15);
		displayParamLabel("dky", hw.display.Width() / 4 + 8, 15);
		displayParamLabel("acc", hw.display.Width() / 2 + 8, 15);
		displayParamLabel("tone", hw.display.Width() * 0.75 + 4, 15);
		displayParamLabel("noiz", 5, 41);

		displayParamLabel("Gain", hw.display.Width() * 0.75 + 4, 41);
		break;
	case 3:
		buildString("Cymbal");
		displayParamLabel("freq", 4, 15);
		displayParamLabel("dky", hw.display.Width() / 4 + 8, 15);
		displayParamLabel("acc", hw.display.Width() / 2 + 8, 15);
		displayParamLabel("tone", hw.display.Width() * 0.75 + 4, 15);
		displayParamLabel("noiz", 5, 41);

		displayParamLabel("Gain", hw.display.Width() * 0.75 + 4, 41);
		break;
	case 4:
		buildString("Master Bus");
		displayParamLabel("cmpT", 5, 15);
		displayParamLabel("cmpR", hw.display.Width() / 4 + 5, 15);
		displayParamLabel("cmpA", hw.display.Width() / 2 + 5, 15);
		displayParamLabel("cmpR", hw.display.Width() * 0.75 + 5, 15);
		displayParamLabel("drv", 8, 41);
		displayParamLabel("tmpo", hw.display.Width() * 0.75 + 4, 41);
		break;
	}
}

void displayDebug() {
	char stepStr[4];
	snprintf(stepStr, 4, "%d", step);
	hw.display.SetCursor(0, 0);
	hw.display.WriteString(stepStr, Font_7x10, true);

	char bassSeqStr[4];
	snprintf(bassSeqStr, 4, "%d", int(drumStates[selectedMenu].kvals[0] * 100));
	hw.display.SetCursor(0, 12);
	hw.display.WriteString(bassSeqStr, Font_7x10, true);

	char tickStr[4];
	snprintf(tickStr, 4, "%d", selectedMenu);
	hw.display.SetCursor(0, 24);
	hw.display.WriteString(tickStr, Font_7x10, true);
}

void setParams() {
	if (drumStates[selectedMenu].isLatched[0]) drumStates[selectedMenu].kvals[0] = hw.knob[0].Process();
	if (drumStates[selectedMenu].isLatched[1]) drumStates[selectedMenu].kvals[1] = hw.knob[1].Process();
	if (drumStates[selectedMenu].isLatched[2]) drumStates[selectedMenu].kvals[2] = hw.knob[2].Process();
	if (drumStates[selectedMenu].isLatched[3]) drumStates[selectedMenu].kvals[3] = hw.knob[3].Process();
	if (drumStates[selectedMenu].isLatched[4]) drumStates[selectedMenu].kvals[4] = hw.knob[4].Process();
	if (drumStates[selectedMenu].isLatched[5]) drumStates[selectedMenu].kvals[5] = hw.knob[5].Process();
	if (drumStates[selectedMenu].isLatched[6]) drumStates[selectedMenu].kvals[6] = hw.knob[6].Process();
	if (drumStates[selectedMenu].isLatched[7]) drumStates[selectedMenu].kvals[7] = hw.knob[7].Process();
}

void setLatch() {
	for(size_t i = 0; i < 8; i++) {
		float prevVal = drumStates[selectedMenu].kvals[i] == 0 ?  0.05 : drumStates[selectedMenu].kvals[i] == 1 ? 0.95 : drumStates[selectedMenu].kvals[i];
		
		if(hw.GetKnobValue(i) > prevVal * 0.95 && hw.GetKnobValue(i) < prevVal * 1.05) drumStates[selectedMenu].isLatched[i] = true;
	}
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	hw.ProcessAllControls();
	handleButton();
	changeMenu();
	for (size_t i = 0; i < size; i++)
	{

		t = tick.Process();

		setLatch();
		setParams();

		switch(selectedMenu) {
			case 0:
			{
				bd.SetFreq(drumStates[0].kvals[0] * 200 + 15);
				bd.SetDecay(drumStates[0].kvals[1]);
				bd.SetDirtiness(drumStates[0].kvals[2]);
				bd.SetTone(drumStates[0].kvals[3]);
				bd.SetAccent(drumStates[0].kvals[4]);
				bd.SetFmEnvelopeAmount(drumStates[0].kvals[5]);
				bd.SetFmEnvelopeDecay(drumStates[0].kvals[6]);

				break;
			}
			case 1:
			{
				sd.SetFreq(drumStates[1].kvals[0] * 800 + 15);
				sd.SetDecay(drumStates[1].kvals[1]);
				sd.SetAccent(drumStates[1].kvals[2]);
				sd.SetSnappy(drumStates[1].kvals[3]);
				sd.SetFmAmount(drumStates[1].kvals[4]);

				break;
			}
			case 2:
			{
				hh.SetFreq(drumStates[2].kvals[0] * 10000);
				hh.SetDecay(drumStates[2].kvals[1]);
				hh.SetAccent(drumStates[2].kvals[2]);
				hh.SetTone(drumStates[2].kvals[3]);
				hh.SetNoisiness(drumStates[2].kvals[4]);

				break;
			}
			case 3:
			{
				cymbal.SetFreq(drumStates[3].kvals[0] * 10000);
				cymbal.SetDecay(drumStates[3].kvals[1] * 2);
				cymbal.SetAccent(drumStates[3].kvals[2]);
				cymbal.SetTone(drumStates[3].kvals[3]);
				cymbal.SetNoisiness(drumStates[3].kvals[4]);

				break;
			}
			case 4:
			{
				float threshRange = fmap(drumStates[4].kvals[0], -80, 0);
				comp.SetThreshold(threshRange);
				float ratioRange = fmap(drumStates[4].kvals[1], 1.0, 40.0);
				comp.SetRatio(ratioRange);
				float attackRange = fmap(drumStates[4].kvals[2], 0.001, 2.0);
				comp.SetAttack(attackRange);
				float relRange = fmap(drumStates[4].kvals[3], 0.001, 3.0);
				comp.SetRelease(relRange);

				float driveRange = fclamp(drumStates[4].kvals[4], 0.25, 0.95);
				drive.SetDrive(driveRange);

				tick.SetFreq(drumStates[4].kvals[7] * 10);

				break;
			}

		}

		bool bassTrig {false};
		bool snareTrig {false};
		bool hatTrig {false};
		bool cymbalTrig {false};

        if(t)
        {
			if(drumStates[0].seq[step]) bassTrig = t;
			if(drumStates[1].seq[step]) snareTrig = t;
			if(drumStates[2].seq[step]) hatTrig = t;
			if(drumStates[3].seq[step]) cymbalTrig = t;
			step = (step + 1) % 16;
        }

		float bassSig = bd.Process(bassTrig);
		float snareSig = sd.Process(snareTrig);
		float hatSig = hh.Process(hatTrig);
		float cymbalSig = cymbal.Process(cymbalTrig);

		float sig = bassSig * drumStates[0].kvals[7];
		sig += snareSig * drumStates[1].kvals[7];
		sig += hatSig * drumStates[2].kvals[7];
		sig += cymbalSig * drumStates[3].kvals[7];

		sig = comp.Process(sig);
		sig = drive.Process(sig);

		limiter.ProcessBlock(&sig, 1, 2);

        out[0][i] = out[1][i] = sig;
	}
}

int main(void)
{
	hw.Init(true);
	hw.SetAudioBlockSize(48); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_32KHZ);

	float sample_rate = hw.AudioSampleRate();

    tick.Init(8.f, sample_rate);

    bd.Init(sample_rate);
    drumStates[0].kvals[0] = 0.15;
	drumStates[0].kvals[1] = 0.6;
	drumStates[0].kvals[2] = 1.0;
	drumStates[0].kvals[3] = 0.38;
	drumStates[0].kvals[4] = 1.0;
	drumStates[0].kvals[5] = 0.23;
	drumStates[0].kvals[6] = 0.56;
	drumStates[0].kvals[7] = 0.5;

	sd.Init(sample_rate);
	drumStates[1].kvals[0] = 0.2;
	drumStates[1].kvals[1] = 0.17;
	drumStates[1].kvals[2] = 1.0;
	drumStates[1].kvals[3] = 0.73;
	drumStates[1].kvals[4] = 0.48;
	drumStates[1].kvals[7] = 0.1;

	hh.Init(sample_rate);
	drumStates[2].kvals[0] = 0.28;
	drumStates[2].kvals[1] = 0.46;
	drumStates[2].kvals[2] = 0.12;
	drumStates[2].kvals[3] = 0.77;
	drumStates[2].kvals[4] = 0.63;
	drumStates[2].kvals[7] = 0.2;

	cymbal.Init(sample_rate);
	drumStates[3].kvals[0] = 0.1;
	drumStates[3].kvals[1] = 0.49;
	drumStates[3].kvals[2] = 0.4;
	drumStates[3].kvals[3] = 0.88;
	drumStates[3].kvals[4] = 0.61;
	drumStates[3].kvals[7] = 0.2;

	comp.Init(sample_rate);
	drive.Init();
	comp.SetThreshold(0.0);
	drive.SetDrive(0.25);
	drumStates[4].kvals[0] = 1.0;
	drumStates[4].kvals[1] = 0.0;
	drumStates[4].kvals[2] = 0.0;
	drumStates[4].kvals[3] = 0.3;
	drumStates[4].kvals[4] = 0.0;
	drumStates[4].kvals[5] = 0.28;

	drumStates[4].kvals[7] = 0.8;

	limiter.Init();

	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	while(1) {
		updateLeds();
		hw.display.Fill(false);
		displayMenu();
		hw.display.Update();
	}
}
