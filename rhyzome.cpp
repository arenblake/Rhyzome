#include "daisy_field.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

CpuLoadMeter loadMeter;

DaisyField hw;
SyntheticBassDrum bd;
SyntheticSnareDrum sd;
HiHat<SquareNoise, LinearVCA, false> hh;
HiHat<RingModNoise, LinearVCA, true> cymbal;
Metro tick;

Compressor comp;
Overdrive drive;
Limiter limiter;

int clockCount{0};
long unsigned int last{System::GetNow()};
long unsigned int pressTime{System::GetNow()};

int step{0};
bool t;
bool locked{false};
bool plockingState{false};
int lockingStep{0};

const int MENU_COUNT{5};
const int MAX_MACROED_PARAMS{12};

struct MenuState
{
	bool seq[16]{false};
	float plockVals[16][8]{0.0F};
	float kvals[8]{0.0F};
	bool isLatched[8]{false};
	bool isLatchedPlock[8]{false};
	bool isMacroed[8]{false};
};

MenuState menustates[MENU_COUNT];

float paramArray[4][8];

int selectedMenu{MENU_COUNT - 1};

int macroPage{0};
int macroedParamCount{0};

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

void handleButton()
{
	int macroIter{0};
	bool anyBtnPressed{false};

	for (size_t i = 0; i < 16; i++)
	{
		if (selectedMenu == MENU_COUNT - 1)
		{
			if (hw.KeyboardRisingEdge(8))
				step = 0;
			if (hw.KeyboardRisingEdge(9))
			{
				step = 0;
				clockCount = 0;
			}
			if (hw.KeyboardRisingEdge(i))
				menustates[selectedMenu].seq[(i + 8) % 16] = !menustates[selectedMenu].seq[(i + 8) % 16];
			if ((i > 3 && i < 8) || i > 11)
			{
				if (hw.KeyboardRisingEdge(i))
				{
					if (!menustates[macroPage].isMacroed[(macroIter + 4) % 8] && macroedParamCount < MAX_MACROED_PARAMS)
					{
						menustates[macroPage].isMacroed[(macroIter + 4) % 8] = !menustates[macroPage].isMacroed[(macroIter + 4) % 8];
						macroedParamCount++;
					}
					else if (menustates[macroPage].isMacroed[(macroIter + 4) % 8])
					{
						menustates[macroPage].isMacroed[(macroIter + 4) % 8] = !menustates[macroPage].isMacroed[(macroIter + 4) % 8];
						macroedParamCount--;
					}
				}
				macroIter++;
			}
		}
		else
		{
			if (hw.KeyboardRisingEdge(i))
				pressTime = System::GetNow();
			if (hw.KeyboardState(i) && menustates[selectedMenu].seq[(i + 8) % 16])
			{
				if (System::GetNow() - pressTime > 500)
				{
					anyBtnPressed = true;
					plockingState = true;
					lockingStep = (i + 8) % 16;
					hw.display.DrawCircle(120, 4, 3, true);
				}
				// hw.display.DrawCircle(100, 4, 3, true);
			}

			// if (hw.KeyboardFallingEdge(i) && !anyBtnPressed)
			// 	menustates[selectedMenu].seq[(i + 8) % 16] = !menustates[selectedMenu].seq[(i + 8) % 16];

			if (hw.KeyboardRisingEdge(i))
				menustates[selectedMenu].seq[(i + 8) % 16] = !menustates[selectedMenu].seq[(i + 8) % 16];
			// {
			// 	if (menustates[selectedMenu].seq[(i + 8) % 16])
			// 	{
			// 		if (!plockingState)
			// 			menustates[selectedMenu].seq[(i + 8) % 16] = false;
			// 	}
			// 	else
			// 	{
			// 		menustates[selectedMenu].seq[(i + 8) % 16] = true;
			// 	}
			// }
		}
	}
	if (!anyBtnPressed)
		plockingState = false;
	macroIter = 0;
	hw.display.DrawCircle(100, 4, 3, anyBtnPressed);
}

void updateLeds()
{
	int macroIter{0};
	for (size_t i = 0; i < 16; i++)
	{
		if (selectedMenu != MENU_COUNT - 1)
		{
			if (menustates[selectedMenu].seq[i] == true)
				hw.led_driver.SetLed(keyboard_leds[i], 1.f);
			if (menustates[selectedMenu].seq[i] != true)
				hw.led_driver.SetLed(keyboard_leds[i], 0.f);
			hw.led_driver.SetLed(keyboard_leds[step], 0.65f);
		}
		else
		{
			hw.led_driver.SetLed(keyboard_leds[2], 0.5f);
			hw.led_driver.SetLed(keyboard_leds[3], 0.5f);
			if ((i >= 0 && i < 2) || (i > 7 && i < 12))
			{
				if (menustates[selectedMenu].seq[i] == true)
					hw.led_driver.SetLed(keyboard_leds[i], 1.f);
				if (menustates[selectedMenu].seq[i] != true)
					hw.led_driver.SetLed(keyboard_leds[i], 0.f);
			}
			if ((i > 3 && i < 8) || (i > 11))
			{
				if (menustates[macroPage].isMacroed[macroIter] == true)
					hw.led_driver.SetLed(keyboard_leds[i], 1.f);
				if (menustates[macroPage].isMacroed[macroIter] != true)
					hw.led_driver.SetLed(keyboard_leds[i], 0.f);
				macroIter++;
			}
		}
	}
	macroIter = 0;
	hw.led_driver.SwapBuffersAndTransmit();
}

void changeMenu()
{
	float prevMenu = selectedMenu;
	if (hw.sw[0].RisingEdge())
		selectedMenu = selectedMenu == 0 ? MENU_COUNT - 2 : (selectedMenu - 1) % (MENU_COUNT - 1);
	if (hw.sw[1].RisingEdge())
		selectedMenu = (selectedMenu + 1) % (MENU_COUNT - 1);
	if (hw.sw[0].Pressed() && hw.sw[1].Pressed() && !locked)
	{
		locked = true;
		if (selectedMenu == MENU_COUNT - 1)
		{
			selectedMenu = 0;
		}
		else
		{
			selectedMenu = MENU_COUNT - 1;
		}
	}
	if (hw.sw[0].FallingEdge() || hw.sw[1].FallingEdge())
		locked = false;

	if (prevMenu != selectedMenu)
	{
		for (size_t i = 0; i < 8; i++)
		{
			menustates[selectedMenu].isLatched[i] = false;
			menustates[selectedMenu].isLatchedPlock[i] = false;
		}
	}

	if (selectedMenu == MENU_COUNT - 1 && hw.KeyboardRisingEdge(10))
		macroPage = macroPage == 0 ? 3 : macroPage - 1;
	if (selectedMenu == MENU_COUNT - 1 && hw.KeyboardRisingEdge(11))
		macroPage = (macroPage + 1) % 4;
}

void buildString(const char lStrToWrt[15])
{
	char lStr[15];
	sprintf(lStr, lStrToWrt);
	hw.display.SetCursor(0, 0);
	hw.display.WriteString(lStr, Font_6x8, true);
}

void displayParamLabel(const char lStrToWrt[4], int curX, int curY)
{
	char lStr[4];
	sprintf(lStr, lStrToWrt);
	hw.display.SetCursor(curX, curY);
	hw.display.WriteString(lStr, Font_6x8, true);
}

void displayParamValues(int knob, int curX, int curY)
{
	char pStr[4];
	int pVal = int((plockingState ? menustates[selectedMenu].plockVals[lockingStep][knob] : menustates[selectedMenu].kvals[knob]) * 100);
	snprintf(pStr, 4, "%d", pVal);
	hw.display.SetCursor(curX, curY);
	hw.display.WriteString(pStr, Font_7x10, true);
}

void displayTransport()
{
	char lStr[8];
	sprintf(lStr, "T:%s", menustates[4].seq[0] ? "Play" : "Stop");
	hw.display.SetCursor(40, 0);
	hw.display.WriteString(lStr, Font_6x8, true);
}

void displayDebug()
{
	const float avgLoad = loadMeter.GetAvgCpuLoad();
	const float maxLoad = loadMeter.GetMaxCpuLoad();
	const float minLoad = loadMeter.GetMinCpuLoad();

	char aStr[4];
	int aVal = int(avgLoad * 100);
	snprintf(aStr, 4, "%d", aVal);
	hw.display.SetCursor(40, 0);
	hw.display.WriteString(aStr, Font_6x8, true);

	char mStr[4];
	int mVal = int(minLoad * 100);
	snprintf(mStr, 4, "%d", mVal);
	hw.display.SetCursor(55, 0);
	hw.display.WriteString(mStr, Font_6x8, true);

	char maxStr[4];
	int maxVal = int(maxLoad * 100);
	snprintf(maxStr, 4, "%d", maxVal);
	hw.display.SetCursor(65, 0);
	hw.display.WriteString(maxStr, Font_6x8, true);
}

void displayMacro()
{
	char lStr[8];
	switch (macroPage)
	{
	case 0:
		sprintf(lStr, "M:Kick");
		break;
	case 1:
		sprintf(lStr, "M:Snr");
		break;
	case 2:
		sprintf(lStr, "M:Hat");
		break;
	case 3:
		sprintf(lStr, "M:Cym");
		break;
	default:
		sprintf(lStr, "Nope");
		break;
	}

	hw.display.SetCursor(80, 0);
	hw.display.WriteString(lStr, Font_6x8, true);
}

void displayMenu()
{
	hw.display.DrawRect(0, 12, hw.display.Width() - 1, hw.display.Height() - 1, true);
	hw.display.DrawLine(0, hw.display.Height() / 2 + 6, hw.display.Width(), hw.display.Height() / 2 + 6, true);
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

	switch (selectedMenu)
	{
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
		buildString("M. Bus");
		displayTransport();
		displayMacro();
		displayParamLabel("cmpT", 5, 15);
		displayParamLabel("cmpR", hw.display.Width() / 4 + 5, 15);
		displayParamLabel("cmpA", hw.display.Width() / 2 + 5, 15);
		displayParamLabel("cmpR", hw.display.Width() * 0.75 + 5, 15);
		displayParamLabel("drv", 8, 41);
		displayParamLabel("macro", hw.display.Width() / 4 + 2, 41);
		displayParamLabel("prob", hw.display.Width() / 2 + 5, 41);
		displayParamLabel("tmpo", hw.display.Width() * 0.75 + 4, 41);
		break;
	}
}

void setParams()
{
	for (size_t i = 0; i < 8; i++)
	{
		if (menustates[selectedMenu].isLatched[i] && !plockingState)
			menustates[selectedMenu].kvals[i] = hw.knob[i].Process();

		if (menustates[selectedMenu].isLatchedPlock[i] && plockingState)
			menustates[selectedMenu].plockVals[lockingStep][i] = hw.knob[i].Process();
	}
}

void setLatch()
{
	for (size_t i = 0; i < 8; i++)
	{
		float prevVal = menustates[selectedMenu].kvals[i] == 0 ? 0.05 : menustates[selectedMenu].kvals[i] == 1 ? 0.95
																											   : menustates[selectedMenu].kvals[i];

		float plockPrevVal = menustates[selectedMenu].plockVals[lockingStep][i] == 0 ? 0.05 : menustates[selectedMenu].plockVals[lockingStep][i] == 1 ? 0.95
																																					  : menustates[selectedMenu].plockVals[lockingStep][i];

		if ((hw.GetKnobValue(i) > prevVal * 0.95 && hw.GetKnobValue(i) < prevVal * 1.05))
			menustates[selectedMenu].isLatched[i] = !plockingState;

		if ((hw.GetKnobValue(i) > plockPrevVal * 0.95 && hw.GetKnobValue(i) < plockPrevVal * 1.05) && plockingState)
			menustates[selectedMenu].isLatchedPlock[i] = true;
	}
}

void handleMidi()
{
	hw.midi.Listen();
	while (hw.midi.HasEvents())
	{
		MidiEvent me = hw.midi.PopEvent();

		if (me.srt_type == Start)
		{
			clockCount = -1;
			step = -1;
			menustates[MENU_COUNT - 1].seq[0] = true;
		}

		if (me.srt_type == Stop)
		{
			clockCount = -1;
			step = -1;
			menustates[MENU_COUNT - 1].seq[0] = false;
		}

		if (me.srt_type == TimingClock && menustates[MENU_COUNT - 1].seq[0])
		{
			clockCount++;
		}

		if (clockCount % 6 == 0 && menustates[MENU_COUNT - 1].seq[0])
		{
			step = (step + 1) % 16;
		}
	}
}

inline float Constrain(float var, float min, float max)
{
	if (var < min)
	{
		var = min;
	}
	else if (var > max)
	{
		var = max;
	}
	return var;
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	loadMeter.OnBlockStart();
	hw.ProcessAllControls();
	handleButton();
	changeMenu();
	setLatch();
	setParams();

	float macroOffset = menustates[4].kvals[5] - 0.5;

	for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 0; j < 8; j++)
		{
			if (menustates[i].plockVals[step][j] != 0.0f && menustates[i].seq[step])
			{
				paramArray[i][j] = menustates[i].plockVals[step][j];
				// paramArray[i][j] = Constrain(menustates[i].isMacroed[j] ? menustates[i].plockVals[step][j] + (macroOffset) : menustates[i].plockVals[step][j], 0.0F, 1.0F);
			}
			else
			{

				paramArray[i][j] = Constrain(menustates[i].isMacroed[j] ? menustates[i].kvals[j] + (macroOffset) : menustates[i].kvals[j], 0.0F, 1.0F);
			}
		}
	}

	bd.SetFreq(paramArray[0][0] * 200 + 15);
	bd.SetDecay(paramArray[0][1]);
	bd.SetDirtiness(paramArray[0][2]);
	bd.SetTone(paramArray[0][3]);
	bd.SetAccent(paramArray[0][4]);
	bd.SetFmEnvelopeAmount(paramArray[0][5]);
	bd.SetFmEnvelopeDecay(paramArray[0][6]);

	sd.SetFreq(paramArray[1][0] * 800 + 15);
	sd.SetDecay(paramArray[1][1]);
	sd.SetAccent(paramArray[1][2]);
	sd.SetSnappy(paramArray[1][3]);
	sd.SetFmAmount(paramArray[1][4]);

	hh.SetFreq(paramArray[2][0] * 10000);
	hh.SetDecay(paramArray[2][1]);
	hh.SetAccent(paramArray[2][2]);
	hh.SetTone(Constrain(paramArray[2][3], 0.0F, 0.93F));
	hh.SetNoisiness(paramArray[2][4]);

	cymbal.SetFreq(paramArray[3][0] * 10000);
	cymbal.SetDecay(paramArray[3][1]);
	cymbal.SetAccent(paramArray[3][2]);
	cymbal.SetTone(paramArray[3][3]);
	cymbal.SetNoisiness(paramArray[3][4]);

	float threshRange = fmap(menustates[4].kvals[0], -80, 0);
	comp.SetThreshold(threshRange);
	float ratioRange = fmap(menustates[MENU_COUNT - 1].kvals[1], 1.0, 40.0);
	comp.SetRatio(ratioRange);
	float attackRange = fmap(menustates[MENU_COUNT - 1].kvals[2], 0.001, 2.0);
	comp.SetAttack(attackRange);
	float relRange = fmap(menustates[MENU_COUNT - 1].kvals[3], 0.001, 3.0);
	comp.SetRelease(relRange);

	float driveRange = fclamp(menustates[MENU_COUNT - 1].kvals[4], 0.25, 0.95);
	drive.SetDrive(driveRange);

	tick.SetFreq(menustates[MENU_COUNT - 1].kvals[7] * 10);

	float randVals[4];

	for (size_t i = 0; i < 4; i++)
	{
		randVals[i] = Random::GetFloat(0.0F, 1.0F);
	}

	for (size_t i = 0; i < size; i++)
	{

		if (menustates[MENU_COUNT - 1].seq[0])
			t = tick.Process();

		bool trigs[4]{false};

		if (menustates[MENU_COUNT - 1].seq[0] && menustates[MENU_COUNT - 1].seq[1])
		{

			if (clockCount % 6 == 0)
			{
				if (System::GetNow() - last > 50)
				{
					for (size_t i = 0; i < 4; i++)
					{
						bool randVal = menustates[MENU_COUNT - 1].seq[i + 8] ? menustates[MENU_COUNT - 1].kvals[6] > randVals[i] * 0.5 : true;
						if (menustates[i].seq[step] && randVal)
							trigs[i] = true;
					}

					last = System::GetNow();
				}
				clockCount = 6;
			}
		}
		else
		{
			if (t)
			{
				for (size_t i = 0; i < 4; i++)
				{
					bool randVal = menustates[MENU_COUNT - 1].seq[i + 8] ? menustates[MENU_COUNT - 1].kvals[6] > randVals[i] * 0.5 : true;
					if (menustates[i].seq[step] && randVal)
						trigs[i] = true;
				}

				step = (step + 1) % 16;
			}
		}

		float bassSig = bd.Process(trigs[0]);
		float snareSig = sd.Process(trigs[1]);
		float hatSig = hh.Process(trigs[2]);
		float cymbalSig = cymbal.Process(trigs[3]);

		float sig = bassSig * menustates[0].kvals[7];
		sig += snareSig * menustates[1].kvals[7];
		sig += hatSig * menustates[2].kvals[7];
		sig += cymbalSig * menustates[3].kvals[7];

		sig = comp.Process(sig);
		sig = drive.Process(sig);

		limiter.ProcessBlock(&sig, 1, 2);

		out[0][i] = out[1][i] = sig;
		loadMeter.OnBlockEnd();
	}
}

int main(void)
{
	hw.Init(true);
	hw.SetAudioBlockSize(32); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_32KHZ);

	float sample_rate = hw.AudioSampleRate();

	loadMeter.Init(sample_rate, hw.AudioBlockSize());
	tick.Init(8.f, sample_rate);

	bd.Init(sample_rate);
	menustates[0].kvals[0] = 0.15;
	menustates[0].kvals[1] = 0.6;
	menustates[0].kvals[2] = 1.0;
	menustates[0].kvals[3] = 0.38;
	menustates[0].kvals[4] = 1.0;
	menustates[0].kvals[5] = 0.23;
	menustates[0].kvals[6] = 0.56;
	menustates[0].kvals[7] = 0.5;

	sd.Init(sample_rate);
	menustates[1].kvals[0] = 0.2;
	menustates[1].kvals[1] = 0.17;
	menustates[1].kvals[2] = 1.0;
	menustates[1].kvals[3] = 0.73;
	menustates[1].kvals[4] = 0.48;
	menustates[1].kvals[7] = 0.1;

	hh.Init(sample_rate);
	menustates[2].kvals[0] = 0.28;
	menustates[2].kvals[1] = 0.46;
	menustates[2].kvals[2] = 0.12;
	menustates[2].kvals[3] = 0.77;
	menustates[2].kvals[4] = 0.63;
	menustates[2].kvals[7] = 0.2;

	cymbal.Init(sample_rate);
	menustates[3].kvals[0] = 0.1;
	menustates[3].kvals[1] = 0.49;
	menustates[3].kvals[2] = 0.4;
	menustates[3].kvals[3] = 0.88;
	menustates[3].kvals[4] = 0.61;
	menustates[3].kvals[7] = 0.2;

	comp.Init(sample_rate);
	drive.Init();
	comp.SetThreshold(0.0);
	drive.SetDrive(0.25);
	menustates[MENU_COUNT - 1].kvals[0] = 1.0;
	menustates[MENU_COUNT - 1].kvals[1] = 0.0;
	menustates[MENU_COUNT - 1].kvals[2] = 0.1;
	menustates[MENU_COUNT - 1].kvals[3] = 0.15;
	menustates[MENU_COUNT - 1].kvals[4] = 0.0;
	menustates[MENU_COUNT - 1].kvals[5] = 0.5;
	menustates[MENU_COUNT - 1].kvals[6] = 1.0;

	menustates[MENU_COUNT - 1].kvals[7] = 0.8;

	limiter.Init();

	hw.StartAdc();
	hw.StartAudio(AudioCallback);

	while (1)
	{
		if (menustates[MENU_COUNT - 1].seq[1])
			handleMidi();

		updateLeds();
		hw.display.Fill(false);
		displayMenu();

		hw.display.Update();
	}
}
