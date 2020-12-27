#include "pch.h"
#include "TPinballTable.h"


#include "control.h"
#include "loader.h"
#include "memory.h"
#include "pb.h"
#include "pinball.h"
#include "render.h"
#include "Sound.h"
#include "TBall.h"
#include "TBlocker.h"
#include "TBumper.h"
#include "TComponentGroup.h"
#include "TDemo.h"
#include "TFlagSpinner.h"
#include "TGate.h"
#include "THole.h"
#include "timer.h"
#include "TKickback.h"
#include "TKickout.h"
#include "TLight.h"
#include "TLightBargraph.h"
#include "TLightGroup.h"
#include "TLightRollover.h"
#include "TOneway.h"
#include "TPopupTarget.h"
#include "TRamp.h"
#include "TRollover.h"
#include "TSink.h"
#include "TSoloTarget.h"
#include "TSound.h"
#include "TTableLayer.h"
#include "TTextBox.h"
#include "TTimer.h"
#include "TTripwire.h"
#include "TWall.h"
#include "TPlunger.h"
#include "TFlipper.h"
#include "TDrain.h"

int TPinballTable::score_multipliers[5] = {1, 2, 3, 5, 10};


TPinballTable::TPinballTable(): TPinballComponent(nullptr, -1, false)
{
	int shortArrLength;

	ComponentList = new objlist_class(32, 16);
	BallList = new objlist_class(3, 1);
	CurScoreStruct = nullptr;
	ScoreBallcount = nullptr;
	ScorePlayerNumber1 = nullptr;
	UnknownP10 = 0;
	UnknownBaseFlag2 = 1;
	TiltLockFlag = 0;
	EndGameTimeoutTimer = 0;
	LightShowTimer = 0;
	ReplayTimer = 0;
	TiltTimeoutTimer = 0;
	MultiballFlag = 0;

	auto ballObj = new TBall(this);
	BallList->Add(ballObj);
	if (ballObj)
		ballObj->UnknownBaseFlag2 = 0;
	new TTableLayer(this);
	LightGroup = new TLightGroup(this, 0);

	auto score1 = score::create("score1", render::background_bitmap);
	CurScoreStruct = score1;
	PlayerScores[0].ScoreStruct = score1;
	for (int scoreIndex = 1; scoreIndex < 4; scoreIndex++)
	{
		PlayerScores[scoreIndex].ScoreStruct = score::dup(CurScoreStruct, scoreIndex);
	}

	CurrentPlayer = 0;
	MaxBallCount = 3;
	ScoreBallcount = score::create("ballcount1", render::background_bitmap);
	ScorePlayerNumber1 = score::create("player_number1", render::background_bitmap);
	int groupIndexObjects = loader::query_handle("table_objects");

	short* shortArr = loader::query_iattribute(groupIndexObjects, 1025, &shortArrLength);
	if (shortArrLength > 0)
	{
		for (int i = 0; i < shortArrLength / 2; ++i)
		{
			int objectType = *shortArr;
			short* shortArrPlus1 = shortArr + 1;
			int groupIndex = *shortArrPlus1;
			shortArr = shortArrPlus1 + 1;
			switch (objectType)
			{
			case 1000:
			case 1010:
				new TWall(this, groupIndex);
				break;
			case 1001:
				Plunger = new TPlunger(this, groupIndex);
				break;
			case 1002:
				new TLight(this, groupIndex);
				//((objlist_class*)(LightGroup + 42))->Add(light);
				break;
			case 1003:
				FlipperL = new TFlipper(this, groupIndex);
				break;
			case 1004:
				FlipperR = new TFlipper(this, groupIndex);
				break;
			case 1005:
				new TBumper(this, groupIndex);
				break;
			case 1006:
				new TPopupTarget(this, groupIndex);
				break;
			case 1007:
				Drain = new TDrain(this, groupIndex);
				break;
			case 1011:
				new TBlocker(this, groupIndex);
				break;
			case 1012:
				new TKickout(this, groupIndex, 1);
				break;
			case 1013:
				new TGate(this, groupIndex);
				break;
			case 1014:
				new TKickback(this, groupIndex);
				break;
			case 1015:
				new TRollover(this, groupIndex);
				break;
			case 1016:
				new TOneway(this, groupIndex);
				break;
			case 1017:
				new TSink(this, groupIndex);
				break;
			case 1018:
				new TFlagSpinner(this, groupIndex);
				break;
			case 1019:
				new TSoloTarget(this, groupIndex);
				break;
			case 1020:
				new TLightRollover(this, groupIndex);
				break;
			case 1021:
				new TRamp(this, groupIndex);
				break;
			case 1022:
				new THole(this, groupIndex);
				break;
			case 1023:
				new TDemo(this, groupIndex);
				break;
			case 1024:
				new TTripwire(this, groupIndex);
				break;
			case 1026:
				new TLightGroup(this, groupIndex);
				break;
			case 1028:
				new TComponentGroup(this, groupIndex);
				break;
			case 1029:
				new TKickout(this, groupIndex, 0);
				break;
			case 1030:
				new TLightBargraph(this, groupIndex);
				break;
			case 1031:
				new TSound(this, groupIndex);
				break;
			case 1032:
				new TTimer(this, groupIndex);
				break;
			case 1033:
				new TTextBox(this, groupIndex);
				break;
			default:
				continue;
			}
		}
	}

	render::build_occlude_list();
	pinball::InfoTextBox = dynamic_cast<TTextBox*>(find_component("info_text_box"));
	pinball::MissTextBox = dynamic_cast<TTextBox*>(find_component("mission_text_box"));
	control::make_links(this);
}


TPinballTable::~TPinballTable()
{
	for (int scoreIndex = 0; scoreIndex < 4; scoreIndex++)
	{
		memory::free(PlayerScores[scoreIndex].ScoreStruct);
	}
	if (ScorePlayerNumber1)
	{
		memory::free(ScorePlayerNumber1);
		ScorePlayerNumber1 = nullptr;
	}
	if (ScoreBallcount)
	{
		memory::free(ScoreBallcount);
		ScoreBallcount = nullptr;
	}
	delete LightGroup;
	while (ComponentList->Count() > 0)
	{
		delete static_cast<TPinballComponent*>(ComponentList->Get(0));
	}
	delete BallList;
	delete ComponentList;
}

TPinballComponent* TPinballTable::find_component(LPCSTR componentName)
{
	int objCount = ComponentList->Count();
	if (objCount > 0)
	{
		for (int index = 0; index < objCount; ++index)
		{
			TPinballComponent* obj = static_cast<TPinballComponent*>(ComponentList->Get(index));
			const CHAR* groupName = obj->GroupName;
			if (groupName && !lstrcmpA(groupName, componentName))
			{
				return obj;
			}
		}
	}
	MessageBoxA(nullptr, "Table cant find:", componentName, 0x2000u);
	return nullptr;
}

TPinballComponent* TPinballTable::find_component(int groupIndex)
{
	char Buffer[33];
	int objCount = ComponentList->Count();
	if (objCount > 0)
	{
		for (int index = 0; index < objCount; ++index)
		{
			TPinballComponent* obj = static_cast<TPinballComponent*>(ComponentList->Get(index));
			if (obj->GroupIndex == groupIndex)
				return obj;
		}
	}
	_itoa_s(groupIndex, Buffer, 10);
	MessageBoxA(nullptr, "Table cant find (lh):", Buffer, 0x2000u);
	return nullptr;
}

int TPinballTable::AddScore(int score)
{
	if (this->ScoreSpecial3Flag)
	{
		this->ScoreSpecial3 += score;
		if (this->ScoreSpecial3 > 5000000)
			this->ScoreSpecial3 = 5000000;
	}
	if (this->ScoreSpecial2Flag)
	{
		this->ScoreSpecial2 += score;
		if (this->ScoreSpecial2 > 5000000)
			this->ScoreSpecial2 = 5000000;
	}
	int addedScore = this->ScoreAdded + score * score_multipliers[this->ScoreMultiplier];
	this->CurScore += addedScore;
	if (this->CurScore > 1000000000)
	{
		++this->CurScoreE9;
		this->CurScore = this->CurScore - 1000000000;
	}
	score::set(this->CurScoreStruct, this->CurScore);
	return addedScore;
}

void TPinballTable::ChangeBallCount(int count)
{
	BallCount = count;
	if (count <= 0)
	{
		score::erase(ScoreBallcount, 1);
	}
	else
	{
		score::set(ScoreBallcount, MaxBallCount - count + 1);
		score::update(ScoreBallcount);
	}
}

void TPinballTable::tilt(float time)
{
	TPinballTable* this2 = this;
	if (!this->TiltLockFlag && !this->UnknownP10)
	{
		pinball::InfoTextBox->Clear();
		pinball::MissTextBox->Clear();
		pinball::InfoTextBox->Display(pinball::get_rc_string(35, 0), -1.0);
		loader::play_sound(this2->SoundIndex3);
		this2->TiltTimeoutTimer = timer::set(30.0, this2, tilt_timeout);

		for (int i = 0; i < ComponentList->Count(); i++)
		{
			static_cast<TPinballComponent*>(ComponentList->Get(i))->Message(1011, time);
		}
		this2->LightGroup->Message(8, 0);
		this2->TiltLockFlag = 1;
		control::table_control_handler(1011);
	}
}


void TPinballTable::port_draw()
{
	for (int index = ComponentList->Count() - 1; index >= 0; index--)
	{
		static_cast<TPinballComponent*>(ComponentList->Get(index))->port_draw();
	}
}

int TPinballTable::Message(int code, float value)
{
	LPSTR rc_text;
	switch (code)
	{
	case 1000:
		if (!TiltLockFlag)
		{
			FlipperL->Message(1, value);
		}
		break;
	case 1001:
		if (!TiltLockFlag)
		{
			FlipperL->Message(2, value);
		}
		break;
	case 1002:
		if (!TiltLockFlag)
		{
			FlipperR->Message(1, value);
		}
		break;
	case 1003:
		if (!TiltLockFlag)
		{
			FlipperR->Message(2, value);
		}
		break;
	case 1004:
	case 1005:
		Plunger->Message(code, value);
		break;
	case 1008:
	case 1009:
	case 1010:
		for (int i = 0; i < ComponentList->Count(); i++)
		{
			static_cast<TPinballComponent*>(ComponentList->Get(i))->Message(code, value);
		}
		break;
	case 1012:
		LightGroup->Message(14, 0.0);
		if (TiltLockFlag)
		{
			TiltLockFlag = 0;
			if (TiltTimeoutTimer)
				timer::kill(TiltTimeoutTimer);
			TiltTimeoutTimer = 0;
		}
		break;
	case 1013:
		LightGroup->Message(34, 0.0);
		LightGroup->Message(20, 0.0);
		Plunger->Message(1016, 0.0);
		if (Demo->UnknownBaseFlag2)
			rc_text = pinball::get_rc_string(30, 0);
		else
			rc_text = pinball::get_rc_string(26, 0);
		pinball::InfoTextBox->Display(rc_text, -1.0);
		if (Demo)
			Demo->Message(1014, 0.0);
		break;
	case 1014:
		if (EndGameTimeoutTimer)
		{
			timer::kill(EndGameTimeoutTimer);
			EndGame_timeout(0, this);
			pb::mode_change(1);
		}
		if (LightShowTimer)
		{
			timer::kill(LightShowTimer);
			LightShowTimer = 0;
			Message(1013, 0.0);
		}
		else
		{
			UnknownP6 = 0;
			Message(1024, 0.0);
			auto ball = static_cast<TBall*>(BallList->Get(0));
			ball->Position.Y = 0.0;
			ball->Position.X = 0.0;
			ball->Position.Z = -0.8f;

			auto playerCount = static_cast<int>(floor(value));
			PlayerCount = playerCount;
			if (playerCount >= 1)
			{
				if (playerCount > 4)
					PlayerCount = 4;
			}
			else
			{
				PlayerCount = 1;
			}

			auto plr1Score = PlayerScores[0].ScoreStruct;
			CurrentPlayer = 0;
			CurScoreStruct = plr1Score;
			CurScore = 0;
			score::set(plr1Score, 0);
			ScoreMultiplier = 0;

			for (int plrIndex = 1; plrIndex < PlayerCount; ++plrIndex)
			{
				auto scorePtr = &PlayerScores[plrIndex];
				score::set(scorePtr->ScoreStruct, 0);
				scorePtr->Score = 0;
				scorePtr->ScoreE9Part = 0;
				scorePtr->BallCount = MaxBallCount;
				scorePtr->ExtraBalls = ExtraBalls;
				scorePtr->BallLockedCounter = BallLockedCounter;
				scorePtr->Unknown2 = ScoreSpecial3;
			}

			BallCount = MaxBallCount;
			Sound::Idle();
			ChangeBallCount(BallCount);
			score::set(ScorePlayerNumber1, CurrentPlayer + 1);
			score::update(ScorePlayerNumber1);
			Sound::Idle();

			for (auto scoreIndex = 4 - PlayerCount; scoreIndex > 0; scoreIndex--)
			{
				score::set(PlayerScores[scoreIndex].ScoreStruct, -1);
			}

			Sound::Idle();
			ScoreSpecial3Flag = 0;
			ScoreSpecial2Flag = 0;
			UnknownP71 = 0;
			pinball::InfoTextBox->Clear();
			Sound::Idle();
			pinball::MissTextBox->Clear();
			Sound::Idle();
			LightGroup->Message(28, 0.2f);
			auto time = loader::play_sound(SoundIndex1);
			LightShowTimer = timer::set(time, this, LightShow_timeout);
		}
		break;
	case 1018:
		if (ReplayTimer)
			timer::kill(ReplayTimer);
		ReplayTimer = timer::set(floor(value), this, replay_timer_callback);
		ReplayActiveFlag = 1;
		break;
	case 1021:
		{
			if (PlayerCount <= 1)
			{
				char* textboxText;
				if (Demo->UnknownBaseFlag2)
					textboxText = pinball::get_rc_string(30, 0);
				else
					textboxText = pinball::get_rc_string(26, 0);
				pinball::InfoTextBox->Display(textboxText, -1.0);
				break;
			}

			auto nextPlayer = (CurrentPlayer + 1) % PlayerCount;
			auto nextScorePtr = &PlayerScores[nextPlayer];
			if (nextScorePtr->BallCount <= 0)
				break;

			PlayerScores[CurrentPlayer].Score = CurScore;
			PlayerScores[CurrentPlayer].ScoreE9Part = CurScoreE9;
			PlayerScores[CurrentPlayer].BallCount = BallCount;
			PlayerScores[CurrentPlayer].ExtraBalls = ExtraBalls;
			PlayerScores[CurrentPlayer].BallLockedCounter = BallLockedCounter;
			PlayerScores[CurrentPlayer].Unknown2 = ScoreSpecial3;

			CurScore = nextScorePtr->Score;
			CurScoreE9 = nextScorePtr->ScoreE9Part;
			BallCount = nextScorePtr->BallCount;
			ExtraBalls = nextScorePtr->ExtraBalls;
			BallLockedCounter = nextScorePtr->BallLockedCounter;
			ScoreSpecial3 = nextScorePtr->Unknown2;

			CurScoreStruct = nextScorePtr->ScoreStruct;
			score::set(CurScoreStruct, CurScore);
			CurScoreStruct->DirtyFlag = true;

			ChangeBallCount(BallCount);
			score::set(ScorePlayerNumber1, nextPlayer + 1);
			score::update(ScorePlayerNumber1);

			for (int i = 0; i < ComponentList->Count(); i++)
			{
				static_cast<TPinballComponent*>(ComponentList->Get(i))->Message(1020, static_cast<float>(nextPlayer));
			}

			char* textboxText = nullptr;
			switch (nextPlayer)
			{
			case 0:
				if (Demo->UnknownBaseFlag2)
					textboxText = pinball::get_rc_string(30, 0);
				else
					textboxText = pinball::get_rc_string(26, 0);
				break;
			case 1:
				if (Demo->UnknownBaseFlag2)
					textboxText = pinball::get_rc_string(31, 0);
				else
					textboxText = pinball::get_rc_string(27, 0);
				break;
			case 2:
				if (Demo->UnknownBaseFlag2)
					textboxText = pinball::get_rc_string(32, 0);
				else
					textboxText = pinball::get_rc_string(28, 0);
				break;
			case 3:
				if (Demo->UnknownBaseFlag2)
					textboxText = pinball::get_rc_string(33, 0);
				else
					textboxText = pinball::get_rc_string(29, 0);
				break;
			default:
				break;
			}

			if (textboxText != nullptr)
				pinball::InfoTextBox->Display(textboxText, -1);
			ScoreSpecial3Flag = 0;
			ScoreSpecial2Flag = 0;
			UnknownP71 = 0;
			CurrentPlayer = nextPlayer;
		}
		break;
	case 1022:
		loader::play_sound(SoundIndex2);
		pinball::MissTextBox->Clear();
		pinball::InfoTextBox->Display(pinball::get_rc_string(34, 0), -1.0);
		EndGameTimeoutTimer = timer::set(3.0, this, EndGame_timeout);
		break;
	case 1024:
		for (int i = 0; i < ComponentList->Count(); i++)
		{
			static_cast<TPinballComponent*>(ComponentList->Get(i))->Message(1024, 0);
		}
		if (ReplayTimer)
			timer::kill(ReplayTimer);
		ReplayTimer = 0;
		if (LightShowTimer)
		{
			timer::kill(LightShowTimer);
			LightGroup->Message(34, 0.0);
		}
		LightShowTimer = 0;
		ScoreMultiplier = 0;
		ScoreAdded = 0;
		ScoreSpecial1 = 0;
		ScoreSpecial2 = 10000;
		ScoreSpecial2Flag = 0;
		ScoreSpecial3 = 20000;
		ScoreSpecial3Flag = 0;
		UnknownP71 = 0;
		ExtraBalls = 0;
		UnknownP75 = 0;
		BallLockedCounter = 0;
		MultiballFlag = 0;
		UnknownP78 = 0;
		ReplayActiveFlag = 0;
		ReplayTimer = 0;
		TiltLockFlag = 0;
		break;

	default: break;
	}

	control::table_control_handler(code);
	return 0;
}

void TPinballTable::EndGame_timeout(int timerId, void* caller)
{
	auto table = static_cast<TPinballTable*>(caller);
	table->EndGameTimeoutTimer = 0;
	pb::end_game();

	for (int i = 0; i < table->ComponentList->Count(); i++)
	{
		static_cast<TPinballComponent*>(table->ComponentList->Get(i))->Message(1022, 0);
	}
	if (table->Demo)
		table->Demo->Message(1022, 0.0);
	control::handler(67, pinball::MissTextBox);
	pinball::InfoTextBox->Display(pinball::get_rc_string(24, 0), -1.0);
}

void TPinballTable::LightShow_timeout(int timerId, void* caller)
{
	auto table = static_cast<TPinballTable*>(caller);
	table->LightShowTimer = 0;
	table->Message(1013, 0.0);
}

void TPinballTable::replay_timer_callback(int timerId, void* caller)
{
	auto table = static_cast<TPinballTable*>(caller);
	table->ReplayActiveFlag = 0;
	table->ReplayTimer = 0;
}

void TPinballTable::tilt_timeout(int timerId, void* caller)
{
	auto table = static_cast<TPinballTable*>(caller);
	table->TiltTimeoutTimer = 0;
	if (table->TiltLockFlag)
	{
		/*v2 = table->ListP2.ListPtr;
		v3 = v2->Array;
		if (v2->Count > 0)
		{
			v4 = v2->Count;
			do
			{
				(*(void(__stdcall**)(void*, char*, char*, _DWORD, _DWORD))(*(_DWORD*)table->Drain + 20))(
					*v3,
					&v5,
					&v5,
					0.0,
					0);
				++v3;
				--v4;
			} while (v4);
		}*/
	}
}