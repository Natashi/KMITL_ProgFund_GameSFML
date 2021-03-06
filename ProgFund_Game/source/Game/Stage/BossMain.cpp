#include "pch.h"

#include "Background1.hpp"
#include "BossMain.hpp"

//----------------------------------------------------------------------------------------------------------

class Spell_BonusScoreText : public TaskBase {
	StaticRenderObject2D objHeader_;
	StaticRenderObject2D objScore_;
	bool bBigBonus_;
public:
	Spell_BonusScoreText(Scene* parent, const std::string& header, uint64_t bonus,
		const DxRectangle<float>& rcDst, D3DCOLOR colTop, D3DCOLOR colBot, bool bBigBonus) : TaskBase(parent) 
	{
		GET_INSTANCE(ResourceManager, resourceManager);

		frameEnd_ = 180;
		bBigBonus_ = bBigBonus;

		VertexTLX verts[4];
		std::vector<VertexTLX> vertex;
		std::vector<uint16_t> index;

		{
			auto textureAscii = resourceManager->GetResourceAs<TextureResource>("img/system/ascii_960.png");
			objHeader_.SetTexture(textureAscii);

			DxRectangle<float> rcBaseDst(-10, -10, 10, 10);

			for (size_t iChar = 0; iChar < header.size(); ++iChar) {
				{
					uint16_t bii = iChar * 4;
					index.push_back(bii + 0);
					index.push_back(bii + 1);
					index.push_back(bii + 2);
					index.push_back(bii + 2);
					index.push_back(bii + 1);
					index.push_back(bii + 3);
				}

				{
					SystemUtility::SetVertexAsciiSingle(verts, header[iChar],
						rcBaseDst + DxRectangle<float>::SetFromSize(16 * iChar, 0), 
						D3DXVECTOR2(21, 21), D3DXVECTOR2(512, 512));
					verts[0].diffuse = (colTop & 0x00ffffff) | (0xff000000);
					verts[1].diffuse = (colTop & 0x00ffffff) | (0xff000000);
					verts[2].diffuse = (colBot & 0x00ffffff) | (0xff000000);
					verts[3].diffuse = (colBot & 0x00ffffff) | (0xff000000);

					{
						vertex.push_back(verts[0]);
						vertex.push_back(verts[1]);
						vertex.push_back(verts[2]);
						vertex.push_back(verts[3]);
					}
				}
			}

			objHeader_.SetArrayVertex(vertex);
			objHeader_.SetArrayIndex(index);

			objHeader_.SetPosition(320 - (16 * (header.size() - 1)) / 2.0f, 192, 1);
		}

		{
			auto textureDigit = resourceManager->GetResourceAs<TextureResource>("img/system/sys_digit.png");
			objScore_.SetTexture(textureDigit);

			vertex.clear();
			index.clear();

			std::string _bonus = std::to_string(bonus);
			_bonus = (char)('0' + 13) + _bonus;
			DxRectangle<float> rcBaseDst(-8, -12, 8, 12);

			for (size_t iDigit = 0; iDigit < _bonus.size(); ++iDigit) {
				{
					uint16_t bii = iDigit * 4;
					index.push_back(bii + 0);
					index.push_back(bii + 1);
					index.push_back(bii + 2);
					index.push_back(bii + 2);
					index.push_back(bii + 1);
					index.push_back(bii + 3);
				}

				{
					DxRectangle<int> rcChar = DxRectangle<int>::SetFromIndex(16, 24, _bonus[iDigit] - '0', 20);
					SystemUtility::SetVertexAsciiSingle(verts, rcChar,
						rcBaseDst + DxRectangle<float>::SetFromSize(16 * iDigit, 0), D3DXVECTOR2(256, 64));
					verts[0].diffuse = (colTop & 0x00ffffff) | (0xff000000);
					verts[1].diffuse = (colTop & 0x00ffffff) | (0xff000000);
					verts[2].diffuse = (colBot & 0x00ffffff) | (0xff000000);
					verts[3].diffuse = (colBot & 0x00ffffff) | (0xff000000);

					{
						vertex.push_back(verts[0]);
						vertex.push_back(verts[1]);
						vertex.push_back(verts[2]);
						vertex.push_back(verts[3]);
					}
				}
			}

			objScore_.SetArrayVertex(vertex);
			objScore_.SetArrayIndex(index);

			objScore_.SetPosition(320 - (16 * (_bonus.size() - 1)) / 2.0f, 192 + 32, 1);
		}
	}

	virtual void Render(byte layer) {
		if (layer != 6) return;
		objHeader_.Render();
		objScore_.Render();
	}
	virtual void Update() {
		GET_INSTANCE(ScriptSoundLibrary, soundLib);
		if (bBigBonus_) {
			if (frame_ == 0)
				soundLib->PlaySE("capture");
			else if (frame_ == 20)
				soundLib->PlaySE("bonus");
		}

		if (frame_ < 20) {
			float tmp = frame_ / 19.0f;
			float tmp_s = Math::Lerp::Smooth<double>(0, 1, tmp);

			objHeader_.SetScaleY(tmp_s);

			objHeader_.SetAlpha(tmp * 255);
			objScore_.SetAlpha(tmp * 255);
		}

		if (frame_ >= 150 && frame_ < 180) {
			float tmp = (frame_ - 150) / 29.0f;
			float tmp_s = Math::Lerp::Smooth<double>(1, 0, tmp);

			objHeader_.SetScaleY(tmp_s);
			objScore_.SetScaleY(tmp_s);
		}

		++frame_;
	}
};

class Spell_BonusWaiter {
	Stage_MainScene* stage_;
	int playerLifePrev_;
	int timerMax_;
	size_t bonus_;
public:
	Spell_BonusWaiter() {
		playerLifePrev_ = 0;
		timerMax_ = -1;
	}

	void Init(Stage_MainScene* stage) {
		stage_ = stage;
	}

	void Begin(Stage_EnemyPhase* pPhase, size_t bonus) {
		if (stage_ == nullptr) return;
		auto objPlayer = stage_->GetPlayer();

		playerLifePrev_ = objPlayer->GetPlayerData()->life;
		timerMax_ = pPhase->timerMax_;
		bonus_ = bonus;
	}
	void End(Stage_EnemyPhase* pPhase) {
		if (stage_ == nullptr) return;
		auto objPlayer = stage_->GetPlayer();

		auto stageUI = stage_->GetParentManager()->GetSceneAs<Stage_MainSceneUI>(Scene::StageUI);

		int playerLifeNow = objPlayer->GetPlayerData()->life;

		size_t addingBonus = 0;
		double mul = (int)(objPlayer->GetPlayerData()->scoreMultiplier * 10) / 10.0;	//Round to 1 decimal place

		//The player must not have lost a life, and the attack must have been shot down before its timer ran out.
		if (playerLifeNow >= playerLifePrev_ && (pPhase->timerMax_ == -1 || pPhase->timer_ > 0)) {
			addingBonus = bonus_;
			if (pPhase->timerMax_ > 0) {
				addingBonus = bonus_ * (0.2 + (pPhase->timer_ / (double)pPhase->timerMax_) * 0.8);
				addingBonus = std::max(addingBonus, 25000U);
			}

			addingBonus *= mul;

			stageUI->AddTask(new Spell_BonusScoreText(stageUI.get(), "Spell Bonus Get!", addingBonus,
				DxRectangle<int>(), D3DCOLOR_XRGB(246, 212, 101), 0xffffffff, true));
		}
		else {		//Fonus Bailed
			addingBonus = 10000 * mul;

			stageUI->AddTask(new Spell_BonusScoreText(stageUI.get(), "Bonus Failed...", addingBonus,
				DxRectangle<int>(), 0xffffffff, D3DCOLOR_XRGB(186, 182, 200), false));
		}

		objPlayer->GetPlayerData()->stat.score += addingBonus;
	}
};
static shared_ptr<Spell_BonusWaiter> g_spellBonusWait = shared_ptr<Spell_BonusWaiter>(new Spell_BonusWaiter());

//----------------------------------------------------------------------------------------------------------

class Shinki_Non1 : public Stage_EnemyPhase {
	int step_;
	int tmpS_[2];
	double tmpD_[2];
public:
	Shinki_Non1(Scene* parent, Stage_EnemyTask_Scripted* objEnemy) : Stage_EnemyPhase(parent, objEnemy) {
		timerDelay_ = 120;
		timerMax_ = timer_ = 30 * 60;
		lifeMax_ = 3400;

		step_ = 0;
		tmpS_[0] = SystemUtility::RandDirection();
		tmpS_[1] = RandProvider::GetBase()->GetInt(0, 1);

		objEnemy->SetX(-999);
		objEnemy->SetY(-999);
		objEnemy->rateShotDamage_ = 0;
	}

	virtual void Activate() {
		Stage_EnemyPhase::Activate();

		{
			parentEnemy_->SetX(92);
			parentEnemy_->SetY(-64);

			Stage_MovePatternLine_Frame* move = new Stage_MovePatternLine_Frame(parentEnemy_);
			move->SetAtFrame(320, 128, 60, Math::Lerp::MODE_DECELERATE);

			parentEnemy_->SetPattern(shared_ptr<Stage_MovePatternLine_Frame>(move));
		}
	}
	virtual void Finalize() {
		GET_INSTANCE(ScriptSoundLibrary, soundLib);
		soundLib->PlaySE("enemydown");
	}

	virtual void Update() {
		GET_INSTANCE(RandProvider, rand);
		GET_INSTANCE(ScriptSoundLibrary, soundLib);
		Stage_MainScene* stage = (Stage_MainScene*)parent_;
		shared_ptr<Stage_ShotManager> shotManager = stage->GetShotManager();
		EnemyBoss_Shinki* objBoss = (EnemyBoss_Shinki*)parentEnemy_;

		switch (step_) {
		case 0:		//Initial wait
			if (frame_ == 100) {
				step_ = 1;
				frame_ = 0;
			}
			break;
		case 1:		//Fire circle
		{
			size_t eFrame = frame_ - 1;
			if (eFrame == 0) {
				objBoss->SetChargeType(0);
				objBoss->SetChargeDuration(24 * 2);
				tmpD_[0] = rand->GetReal(0, GM_PI_X2);
				tmpD_[1] = 0;

				soundLib->PlaySE("charge");
			}

			if (eFrame % 2 == 0) {
				double rFrame = (eFrame / 2) / 23.0;
				for (int i = 0; i < 32; ++i) {
					bool bPolar = i % 2 == 0;
					double sa = tmpD_[0] + GM_PI_X2 / 32 * i + tmpD_[1];
					double sr = Math::Lerp::Linear<double>(64, -32, rFrame);
					shotManager->AddEnemyShot(OffsetPos(parentEnemy_->GetMovePosition(), sa, sr),
						Math::Lerp::Linear<double>(0.5, 5.5, rFrame), sa,
						bPolar ? ShotConst::RedBallM : ShotConst::WhiteBallM, 8,
						bPolar ? IntersectPolarity::Black : IntersectPolarity::White);
				}
				soundLib->PlaySE("shot1", 70);
				tmpD_[1] += GM_DTORA(1.32) * tmpS_[0];
			}

			if (eFrame == 23 * 2 + 6) {
				step_ = 2;
				frame_ = 0;

				tmpS_[0] *= -1;
			}
			break;
		}
		case 2:		//Movement
		{
			objBoss->rateShotDamage_ = 1;

			size_t eFrame = frame_ - 1;
			if (eFrame == 0) {
				Stage_MovePatternLine_Frame* move = new Stage_MovePatternLine_Frame(parentEnemy_);
				move->SetAtFrame(320 + rand->GetReal(-160, 160), rand->GetReal(48, 96), 40, Math::Lerp::MODE_DECELERATE);
				parentEnemy_->SetPattern(move);
			}
			else if (eFrame == 80) {
				step_ = 3;
				frame_ = 0;
			}
			break;
		}
		case 3:		//Fire fans
		{
			struct Stack {
				bool pola;
				size_t cStack;
				float speed[2];
				Stack(bool p, size_t a, float b, float c) : pola(p), cStack(a) { speed[0] = b; speed[1] = c; }
			};
			struct Fan {
				size_t cWay;
				float gapAng;
				std::vector<Stack> listStackArg;
			};

			static std::vector<Fan> listFanArg;
			static size_t iFan;

			size_t eFrame = frame_ - 1;
			if (eFrame == 0) {
				objBoss->SetChargeType(1);
				objBoss->SetChargeDuration(100);

				if (listFanArg.size() == 0) {
					listFanArg = {
						{ 1, GM_DTORA(0), {
							Stack(true, 6, 3, 6) } },
						{ 2, GM_DTORA(40), { Stack(false, 8, 2.5, 5.5), 
							Stack(false, 8, 2.5, 5.5) } },
						{ 5, GM_DTORA(20), { Stack(false, 4, 2, 3.6), Stack(true, 6, 3, 5),
							Stack(false, 10, 4, 7.5), Stack(true, 6, 3, 5), Stack(false, 4, 2, 3.6) } },
					};
				}
				iFan = 0;
			}

			if (iFan < listFanArg.size() && eFrame == iFan * 30) {
				Fan* pFan = &listFanArg[iFan];

				double angToPlayer = parentEnemy_->GetDeltaAngle(stage->GetPlayer().get());
				float ang_off_way = (float)(pFan->cWay / 2U) - (pFan->cWay % 2U == 0U ? 0.5 : 0.0);
				for (size_t iWay = 0U; iWay < pFan->cWay; ++iWay) {
					float sa = angToPlayer + (iWay - ang_off_way) * pFan->gapAng;
					Stack* pStack = &pFan->listStackArg[iWay];
					for (size_t iStack = 0U; iStack < pStack->cStack; ++iStack) {
						bool bPol = (byte)pStack->pola ^ (byte)tmpS_[1];

						float ss = pStack->speed[0];
						if (pStack->cStack > 1U)
							ss += (pStack->speed[1] - pStack->speed[0]) * (iStack / (float)(pStack->cStack - 1U));

						shotManager->AddEnemyShot(parentEnemy_->GetMovePosition(), ss, sa,
							bPol ? ShotConst::WhiteScale : ShotConst::RedScale, 12,
							bPol ? IntersectPolarity::White : IntersectPolarity::Black);
					}
				}

				soundLib->PlaySE("shot2", 90);
				++iFan;
			}

			if (eFrame == listFanArg.size() * 30 + 2) {
				step_ = 4;
				frame_ = 0;

				tmpS_[1] = 1 - tmpS_[1];
			}
			break;
		}
		case 4:		//Movement + end
		{
			size_t eFrame = frame_ - 1;

			if (eFrame == 0) {
				Stage_MovePatternLine_Frame* move = new Stage_MovePatternLine_Frame(parentEnemy_);
				move->SetAtFrame(320 + rand->GetReal(-140, 140), rand->GetReal(64, 144), 40, Math::Lerp::MODE_DECELERATE);
				parentEnemy_->SetPattern(move);
			}
			else if (eFrame == 90) {
				step_ = 0;
				frame_ = 99;
			}
			break;
		}
		}

		Stage_EnemyPhase::Update();
	}
};

//----------------------------------------------------------------------------------------------------------

class Shinki_Spell1 : public Stage_EnemyPhase {
private:
	class _OrbFamiliar : public TaskBase {
		friend class Shinki_Spell1;
	private:
		Stage_ObjMove* objMoveParent_;
		bool bEnd_;
		bool bShoot_;
		bool bWhite_;
		double dir_;

		double angle_[3];
		double radius_;
		Sprite2D objCircle_;
	public:
		_OrbFamiliar(Scene* parent, Stage_ObjMove* moveParent, bool bWhite,
			double iniAngle, double dir) : TaskBase(parent) 
		{
			objMoveParent_ = moveParent;
			bEnd_ = false;
			bShoot_ = false;
			bWhite_ = bWhite;

			angle_[0] = iniAngle;
			angle_[1] = dir;
			angle_[2] = -999;
			radius_ = 0;

			{
				GET_INSTANCE(ResourceManager, resourceManager);

				auto textureOrb = resourceManager->GetResourceAs<TextureResource>("img/stage/enm_orb.png");
				objCircle_.SetTexture(textureOrb);
				objCircle_.SetSourceRect(DxRectangle<int>::SetFromIndex(32, 32, bWhite ? 12 : 10, 10));
				objCircle_.SetDestCenter();
				objCircle_.UpdateVertexBuffer();
			}
		}

		D3DXVECTOR2 GetPosition() {
			D3DXVECTOR2 opos = objMoveParent_->GetMovePosition();
			double pa = angle_[0];
			return opos + (D3DXVECTOR2(cosf(angle_[0]), sinf(angle_[0])) * radius_);
		}

		virtual void Render(byte layer) {
			if (layer != LAYER_ENEMY) return;
			objCircle_.Render();
		}
		virtual void Update() {
			GET_INSTANCE(ScriptSoundLibrary, soundLib);

			if (!bEnd_) {
				if (frame_ < 60) {
					double tmp = frame_ / 59.0;
					constexpr double fHalfFac = 0.6;
					double iLine = 0;
					if (tmp <= fHalfFac)
						iLine = Math::Lerp::Decelerate<double>(0, 1.4, tmp / fHalfFac);
					else
						iLine = Math::Lerp::Accelerate<double>(1.4, 1, (tmp - fHalfFac) / (1 - fHalfFac));
					radius_ = iLine * 86;

					double tmp_sc = Math::Lerp::Smooth<double>(0, 1, tmp);
					objCircle_.SetScale(tmp_sc, tmp_sc, 1);
				}
				objCircle_.SetPosition(this->GetPosition());

				if (bShoot_) {
					Stage_MainScene* stage = (Stage_MainScene*)parent_;
					shared_ptr<Stage_ShotManager> shotManager = stage->GetShotManager();

					double angToPlayer = objMoveParent_->GetDeltaAngle(stage->GetPlayer().get());
					if (angle_[2] == -999) {
						angle_[2] = angToPlayer;
					}
					{
						double diff = Math::AngleDifferenceRad(angle_[2], angToPlayer);
						double turnRate = abs(diff) > GM_DTORA(12) ? (1 / 3.0) : (1 / 15.0);
						angle_[2] = Math::NormalizeAngleRad(angle_[2] + diff * turnRate);
					}

					if (frame_ % 2 == 0) {
						shotManager->AddEnemyShot(D3DXVECTOR2(objCircle_.GetPosition()),
							9, angle_[2],
							!bWhite_ ? ShotConst::RedBullet : ShotConst::WhiteBullet, 4,
							!bWhite_ ? IntersectPolarity::Black : IntersectPolarity::White);

						soundLib->PlaySE("shot2", 30);
					}
				}
			}
			else {
				if (frameEnd_ == UINT_MAX) {
					frame_ = 0;
					frameEnd_ = 26;
				}

				double tmp = frame_ / 26.0;
				double tmp_sc = Math::Lerp::Accelerate<double>(1, 0, tmp);
				objCircle_.SetScale(tmp_sc, tmp_sc, 1);
			}

			objCircle_.SetAngleZ(angle_[0] * 1.3214);

			angle_[0] += GM_DTORA(0.67) * angle_[1];
			++frame_;
		}
	};
private:
	int step_;
	std::vector<shared_ptr<_OrbFamiliar>> listTaskOrbs_;
	double tmp_[2];
public:
	Shinki_Spell1(Scene* parent, Stage_EnemyTask_Scripted* objEnemy) : Stage_EnemyPhase(parent, objEnemy) {
		timerDelay_ = 180;
		timerMax_ = timer_ = 50 * 60;
		lifeMax_ = 8500;

		step_ = 0;
		tmp_[0] = 1;
	}

	virtual void Activate() {
		Stage_EnemyPhase::Activate();

		Stage_MainScene* stage = (Stage_MainScene*)parent_;
		shared_ptr<Stage_ShotManager> shotManager = stage->GetShotManager();
		shotManager->DeleteInCircle(ShotOwnerType::Enemy, 320, 240, 1000, true);

		EnemyBoss_Shinki* objBoss = (EnemyBoss_Shinki*)parentEnemy_;
		parentEnemy_->SetPattern(nullptr);
		parentEnemy_->rateShotDamage_ = 0;

		g_spellBonusWait->Begin(this, 300000);
	}
	virtual void Finalize() {
		EnemyBoss_Shinki* objBoss = (EnemyBoss_Shinki*)parentEnemy_;
		objBoss->SetSpellBackground(false);

		for (auto& iObj : listTaskOrbs_)
			iObj->bEnd_ = true;

		g_spellBonusWait->End(this);

		GET_INSTANCE(ScriptSoundLibrary, soundLib);
		soundLib->PlaySE("enemydown");
	}

	virtual void Update() {
		GET_INSTANCE(ScriptSoundLibrary, soundLib);
		GET_INSTANCE(RandProvider, rand);
		Stage_MainScene* stage = (Stage_MainScene*)parent_;
		shared_ptr<Stage_ShotManager> shotManager = stage->GetShotManager();
		EnemyBoss_Shinki* objBoss = (EnemyBoss_Shinki*)parentEnemy_;

		switch (step_) {
		case 0:
			if (frame_ == 120) {
				Stage_MovePatternLine_Frame* move = new Stage_MovePatternLine_Frame(parentEnemy_);
				move->SetAtFrame(320, 112, 60, Math::Lerp::MODE_SMOOTH);
				parentEnemy_->SetPattern(move);
			}
			else if (frame_ == 190) {
				objBoss->SetSpellBackground(true);
				objBoss->rateShotDamage_ = 1;

				soundLib->PlaySE("spellcard");

				auto objPlayer = stage->GetPlayer();

				double randAng = GM_DTORA(rand->GetReal(-2, 2));

				listTaskOrbs_.resize(4U);
				listTaskOrbs_[0] = shared_ptr<_OrbFamiliar>(new _OrbFamiliar(parent_, parentEnemy_, true, GM_DTORA(-21), 1));
				listTaskOrbs_[2] = shared_ptr<_OrbFamiliar>(new _OrbFamiliar(parent_, parentEnemy_, true, GM_DTORA(159), 1));
				listTaskOrbs_[1] = shared_ptr<_OrbFamiliar>(new _OrbFamiliar(parent_, parentEnemy_, false, GM_DTORA(119), -1));
				listTaskOrbs_[3] = shared_ptr<_OrbFamiliar>(new _OrbFamiliar(parent_, parentEnemy_, false, GM_DTORA(-61), -1));
				for (auto& iObj : listTaskOrbs_) {
					iObj->angle_[0] += randAng;
					parent_->AddTask(iObj);
				}
			}
			else if (frame_ == 200) {
				soundLib->PlaySE("boon1");
			}
			else if (frame_ == 250) {
				step_ = 1;
				frame_ = 0;
			}
			break;
		case 1:
		{
			size_t eFrame = frame_ - 1;
			if (eFrame == 0) {
				objBoss->SetChargeType(0);
				objBoss->SetChargeDuration(210);
			}
			if (frame_ == 10) {
				for (auto& iObj : listTaskOrbs_)
					iObj->bShoot_ = true;
			}

			if (eFrame <= 230 && eFrame % 2 == 0) {
				double angVari = GM_DTORA(Math::Lerp::Linear<double>(110, 45, eFrame / 230.0));
				double angleToPlayer = objBoss->GetDeltaAngle(stage->GetPlayer().get());
				{
					auto shot = shotManager->AddEnemyShot(
						objBoss->GetMovePosition() + D3DXVECTOR2(-14 * tmp_[0], 0),
						0.1, angleToPlayer + rand->GetReal(-angVari, angVari),
						ShotConst::RedRingBallM, 12, IntersectPolarity::Black);
					auto pattern = new Stage_MovePatternAngle(shot.get());
					pattern->SetSpeed(rand->GetReal(2, 3.5));
					pattern->SetDirectionAngleDirect(Stage_MovePattern::NO_CHANGE);
					shot->AddPattern(8, shared_ptr<Stage_MovePattern>(pattern));
				}
				{
					auto shot = shotManager->AddEnemyShot(
						objBoss->GetMovePosition() + D3DXVECTOR2(14 * tmp_[0], 0),
						0.1, angleToPlayer + rand->GetReal(-angVari, angVari),
						ShotConst::WhiteRingBallM, 12, IntersectPolarity::White);
					auto pattern = new Stage_MovePatternAngle(shot.get());
					pattern->SetSpeed(rand->GetReal(2, 3.5));
					pattern->SetDirectionAngleDirect(Stage_MovePattern::NO_CHANGE);
					shot->AddPattern(8, shared_ptr<Stage_MovePattern>(pattern));
				}

				soundLib->PlaySE("shot1");
			}

			if (eFrame > 0 && eFrame % 50 == 0 && eFrame <= 250) {
				float rx = 0;
				while (abs(rx) < 24)
					rx = rand->GetReal(-60, 60);
				Stage_MovePatternLine_Frame* move = new Stage_MovePatternLine_Frame(parentEnemy_);
				move->SetAtFrame(parentEnemy_->posX_ + rx, rand->GetReal(64, 144), 49, Math::Lerp::MODE_DECELERATE);
				parentEnemy_->SetPattern(move);
			}
			if (eFrame == 250 + 60) {
				Stage_MovePatternLine_Frame* move = new Stage_MovePatternLine_Frame(parentEnemy_);
				move->SetAtFrame(320, 112, 40, Math::Lerp::MODE_DECELERATE);
				parentEnemy_->SetPattern(move);
			}
			
			if (eFrame == 250 + 60 + 90) {
				step_ = 2;
				frame_ = 0;
				tmp_[0] = -tmp_[0];
			}
			break;
		}
		case 2:
		{
			size_t eFrame = frame_ - 1;
			if (eFrame % 30 == 0 && eFrame <= 30 * 7) {
				double angleToPlayer = objBoss->GetDeltaAngle(stage->GetPlayer().get()) + GM_DTORA(rand->GetReal(-5, 5));

				bool bWhite = rand->GetInt() % 2 == 0;
				for (int i = -6; i < 7; ++i) {
					shotManager->AddEnemyShot(objBoss->GetMovePosition(), 1.9, 
						angleToPlayer + i * GM_DTORA(7),
						bWhite ? ShotConst::WhiteBallL : RedBallL, 16,
						bWhite ? IntersectPolarity::White : IntersectPolarity::Black);
				}

				soundLib->PlaySE("shot3", 70);
			}

			if (eFrame == 30 * 7 + 120) {
				step_ = 1;
				frame_ = 0;
			}
			break;
		}
		}

		Stage_EnemyPhase::Update();
	}
};

//----------------------------------------------------------------------------------------------------------

class Shinki_Spell2 : public Stage_EnemyPhase {
private:
	int step_;
	double tmp_[3];
public:
	Shinki_Spell2(Scene* parent, Stage_EnemyTask_Scripted* objEnemy) : Stage_EnemyPhase(parent, objEnemy) {
		GET_INSTANCE(RandProvider, rand);

		timerDelay_ = 180;
		timerMax_ = timer_ = 60 * 60;
		lifeMax_ = 7500;

		step_ = 0;
		tmp_[0] = rand->GetReal(0, GM_PI_X2);
		tmp_[1] = SystemUtility::RandDirection();
		tmp_[2] = SystemUtility::RandDirection();
	}

	virtual void Activate() {
		Stage_EnemyPhase::Activate();

		Stage_MainScene* stage = (Stage_MainScene*)parent_;
		shared_ptr<Stage_ShotManager> shotManager = stage->GetShotManager();
		shotManager->DeleteInCircle(ShotOwnerType::Enemy, 320, 240, 1000, true);

		EnemyBoss_Shinki* objBoss = (EnemyBoss_Shinki*)parentEnemy_;
		parentEnemy_->SetPattern(nullptr);
		parentEnemy_->rateShotDamage_ = 0;

		g_spellBonusWait->Begin(this, 500000);
	}
	virtual void Finalize() {
		Stage_MainScene* stage = (Stage_MainScene*)parent_;
		//shared_ptr<Stage_ShotManager> shotManager = stage->GetShotManager();
		//shotManager->DeleteInCircle(ShotOwnerType::Enemy, 320, 240, 1000, true);

		EnemyBoss_Shinki* objBoss = (EnemyBoss_Shinki*)parentEnemy_;
		objBoss->SetSpellBackground(false);
		objBoss->SetChargeDuration(0);

		g_spellBonusWait->End(this);

		GET_INSTANCE(ScriptSoundLibrary, soundLib);
		//soundLib->PlaySE("enemydown");
	}

	virtual void Update() {
		GET_INSTANCE(ScriptSoundLibrary, soundLib);
		GET_INSTANCE(RandProvider, rand);
		Stage_MainScene* stage = (Stage_MainScene*)parent_;
		shared_ptr<Stage_ShotManager> shotManager = stage->GetShotManager();
		EnemyBoss_Shinki* objBoss = (EnemyBoss_Shinki*)parentEnemy_;

		switch (step_) {
		case 0:
			if (frame_ == 90) {
				Stage_MovePatternLine_Frame* move = new Stage_MovePatternLine_Frame(parentEnemy_);
				move->SetAtFrame(320, 80, 60, Math::Lerp::MODE_SMOOTH);
				parentEnemy_->SetPattern(move);
			}
			else if (frame_ == 120) {
				objBoss->SetSpellBackground(true);

				soundLib->PlaySE("spellcard");
			}
			else if (frame_ == 180) {
				objBoss->rateShotDamage_ = 1;

				step_ = 1;
				frame_ = 0;
			}
			break;
		case 1:
		{
			size_t eFrame = frame_ - 1;
			if (eFrame == 0) {
				objBoss->SetChargeType(0);
				objBoss->SetChargeDuration(60 * 6);
			}

			if (eFrame < 60 * 7) {
				if (eFrame >= 40) {
					//if (eFrame % 2 == 0) {
						for (int p = 0; p < 2; ++p) {
							int dir = p * 2 - 1;
							float oy = 192 * sinf(tmp_[0]);
							for (int i = 0; i < 1; ++i) {
								auto shot = shotManager->AddEnemyShot(
									D3DXVECTOR2(320, 240) + D3DXVECTOR2(316 * dir, oy * dir),
									rand->GetReal(1.5, 2),
									rand->GetReal(-GM_PI_2, GM_PI_2) * 0.92f + (p * GM_DTORA(180)),
									p ? ShotConst::RedFire : ShotConst::WhiteFire, 16,
									p ? IntersectPolarity::Black : IntersectPolarity::White);
								shot->SetRenderPriority(LAYER_SHOT - 1);
								shot->SetBlendType(BlendMode::Add);
							}
						}
					//}
					tmp_[0] += GM_DTORA(1) * tmp_[1];

					soundLib->PlaySE("shot1", 65);
				}

				if (eFrame >= 40 + 90 && (eFrame - 130) % 60 == 0) {
					double iniAngle = rand->GetReal(0, GM_PI_X2);

					for (int i = 0; i < 30; ++i) {
						bool bPolar = rand->GetInt() % 2 == 0;
						double sa = iniAngle + GM_PI_X2 / 30 * i;
						auto shot = shotManager->AddEnemyShot(OffsetPos(parentEnemy_->GetMovePosition(), sa, -64),
							4, sa,
							bPolar ? ShotConst::RedRingBallM : ShotConst::WhiteRingBallM, 16,
							bPolar ? IntersectPolarity::Black : IntersectPolarity::White);
						{
							//double angV = rand->GetReal(3, 6);
							double angV = 3.6;
							auto pattern = new Stage_MovePatternAngle(shot.get());
							pattern->SetSpeed(Stage_MovePattern::NO_CHANGE);
							pattern->SetDirectionAngleDirect(Stage_MovePattern::NO_CHANGE);
							pattern->angularVelocity_ = GM_DTORA(bPolar ? angV : -angV) * tmp_[2];
							shot->AddPattern(12, shared_ptr<Stage_MovePattern>(pattern));
						}
						{
							auto pattern = new Stage_MovePatternAngle(shot.get());
							pattern->SetSpeed(Stage_MovePattern::NO_CHANGE);
							pattern->acceleration_ = -2 / 30.0;
							pattern->maxSpeed_ = 2;
							pattern->SetDirectionAngleDirect(Stage_MovePattern::NO_CHANGE);
							shot->AddPattern(30, shared_ptr<Stage_MovePattern>(pattern));
						}
					}
					tmp_[2] = -tmp_[2];

					soundLib->PlaySE("shot2", 80);
				}
			}
			else if (eFrame == 60 * 7 + 80) {
				tmp_[0] = rand->GetReal(0, GM_PI_X2);
				tmp_[1] = SystemUtility::RandDirection();

				step_ = 1;
				frame_ = 0;
			}

			break;
		}
		}

		Stage_EnemyPhase::Update();
	}
};

//----------------------------------------------------------------------------------------------------------

class Shinki_DeadTask : public Stage_EnemyPhase {
	friend class EnemyBoss_Shinki;
private:
	struct ParticleData {
		D3DXVECTOR2 pos;
		float speed;
		float angle;
		D3DXVECTOR2 angleZ;
		float scale;
		size_t frame;
		size_t frameEnd;
	};
	class Particle : public TaskBase {
	private:
		Sprite2D objEffect_;
		D3DXVECTOR2 pos_;
		D3DXVECTOR2 move_;
		D3DXVECTOR3 angle_;
		D3DXVECTOR3 angleAdd_;
		float scale_;
		size_t frameMain_;
	public:
		Particle(Scene* parent, CD3DXVECTOR2 pos, CD3DXVECTOR2 move, CD3DXVECTOR3 angle, 
			CD3DXVECTOR3 angleAdd, float scale, D3DCOLOR color, size_t frameEnd) : TaskBase(parent)
		{
			GET_INSTANCE(ResourceManager, resourceManager);

			auto textureParticle = resourceManager->GetResourceAs<TextureResource>("img/system/eff_particle.png");
			objEffect_.SetTexture(textureParticle);
			objEffect_.SetSourceRect(DxRectangle<int>(0, 0, 64, 64));
			objEffect_.SetDestCenter();
			objEffect_.UpdateVertexBuffer();

			objEffect_.SetColor(color);
			objEffect_.SetScale(scale, scale, 1);

			objEffect_.SetBlendType(BlendMode::Add);

			pos_ = pos;
			move_ = move;
			angle_ = angle;
			angleAdd_ = angleAdd;
			scale_ = scale;
			frameMain_ = frameEnd;
			frameEnd_ = frameMain_ + 20 + 12;
		}

		virtual void Render(byte layer) {
			if (layer != LAYER_EX_UI - 1) return;
			objEffect_.Render();
		}
		virtual void Update() {
			if (frame_ < 12) {
				float tmp = frame_ / 11.0f;
				objEffect_.SetAlpha(tmp * 255);
			}
			else if (frame_ >= 12 + frameMain_) {
				float tmp = (frame_ - (12 + frameMain_)) / 20.0f;
				float tmp_s = Math::Lerp::Smooth<float>(scale_, 0, tmp);
				objEffect_.SetScale(tmp_s, tmp_s, 1);
				objEffect_.SetAlpha((1 - tmp) * 255);
			}

			objEffect_.SetPosition(pos_);
			objEffect_.SetAngle(angle_);

			pos_ += D3DXVECTOR2(cosf(move_[1]), sinf(move_[1])) * move_[0];
			angle_ += angleAdd_;
			++frame_;
		}
	};

	D3DXVECTOR2 posBoss_;
public:
	Shinki_DeadTask(Scene* parent, Stage_EnemyTask_Scripted* objEnemy) : Stage_EnemyPhase(parent, objEnemy) {
		GET_INSTANCE(RandProvider, rand);
	
		lifeMax_ = 100000;
		timer_ = 150;
	}

	virtual void Activate() {
		Stage_EnemyPhase::Activate();

		EnemyBoss_Shinki* objBoss = (EnemyBoss_Shinki*)parentEnemy_;
		objBoss->SetSpellBackground(false);
		objBoss->SetChargeDuration(0);

		objBoss->SetPattern(nullptr);
		objBoss->rateShotDamage_ = 0;

		objBoss->life_ = 0;

		posBoss_ = objBoss->GetMovePosition();
	}

	virtual void Update() {
		GET_INSTANCE(ScriptSoundLibrary, soundLib);
		GET_INSTANCE(RandProvider, rand);

		Stage_MainScene* stage = (Stage_MainScene*)parent_;
		shared_ptr<Stage_ShotManager> shotManager = stage->GetShotManager();
		EnemyBoss_Shinki* objBoss = (EnemyBoss_Shinki*)parentEnemy_;

		if (frame_ == 0) {
			Stage_MovePatternLine_Frame* move = new Stage_MovePatternLine_Frame(parentEnemy_);
			move->SetAtFrame(posBoss_.x + rand->GetReal(-48, 48), posBoss_.y + rand->GetReal(-24, 64), 
				100, Math::Lerp::MODE_DECELERATE);
			parentEnemy_->SetPattern(move);

			parent_->AddTask(new SystemEffects::Explosion(parent_, objBoss->GetMovePosition(), 
				256, D3DCOLOR_XRGB(216, 115, 117), 45));

			soundLib->PlaySE("flare");
		}
		if (frame_ <= 70) {
			float radius = Math::Lerp::Accelerate(64, 1000, frame_ / 70.0);
			shotManager->DeleteInCircle(ShotOwnerType::Enemy, posBoss_.x, posBoss_.y, radius, true);
		}
		if (frame_ == 90) {
			soundLib->PlaySE("charge");
		}
		if (frame_ == 110) {
			objBoss->SetChargeType(0);
			objBoss->SetChargeDuration(0);
		}

		{
			D3DXVECTOR2 pos = objBoss->GetMovePosition();
			auto _AddParticle = [&](CD3DXVECTOR2 rndPos, float speed, float scale, D3DCOLOR color, size_t life) {
				constexpr float spin = GM_DTORA(3);
				parent_->AddTask(new Particle(parent_, pos + rndPos, D3DXVECTOR2(speed, rand->GetReal(0, GM_PI_X2)),
					D3DXVECTOR3(rand->GetReal(0, GM_PI_X2), rand->GetReal(0, GM_PI_X2), rand->GetReal(0, GM_PI_X2)),
					D3DXVECTOR3(rand->GetReal(-spin, spin), rand->GetReal(-spin, spin), rand->GetReal(-spin, spin)),
					scale, color, life));
			};

			
			if (frame_ == 0) {
				for (int i = 0; i < 24; ++i) {
					_AddParticle(D3DXVECTOR2(rand->GetReal(-8, 8), rand->GetReal(-8, 8)),
						rand->GetReal(2.4, 4), rand->GetReal(1, 1.7), 0xffffff, rand->GetInt(80, 100));
				}
			}
			if (frame_ < 140) {
				_AddParticle(D3DXVECTOR2(rand->GetReal(-8, 8), rand->GetReal(-8, 8)),
					rand->GetReal(3.2, 4.3), rand->GetReal(0.7, 1.25), 0xffffff, rand->GetInt(45, 60));
			}
			if (frame_ == 150) {
				for (int i = 0; i < 64; ++i) {
					_AddParticle(D3DXVECTOR2(rand->GetReal(-12, 12), rand->GetReal(-12, 12)),
						rand->GetReal(1.3, 5), rand->GetReal(0.8, 2), D3DCOLOR_XRGB(255, 32, 32), rand->GetInt(80, 150));
				}
			}
		}
		
		if (timer_ == 0) {
			parent_->AddTask(new SystemEffects::Explosion(parent_, objBoss->GetMovePosition(), 
				400, D3DCOLOR_XRGB(255, 255, 255), 65));

			bFinish_ = true;

			soundLib->PlaySE("bossdown");
		}
		--timer_;
		++frame_;
	}
};

//*******************************************************************
//EnemyBoss_Shinki
//*******************************************************************
EnemyBoss_Shinki::EnemyBoss_Shinki(Scene* parent) : Stage_EnemyTask_Scripted(parent) {
	rateShotDamage_ = 0;

	frameAnimLeft_ = 0;
	frameAnimRight_ = 0;
	frameAnimCharge_ = 0;
	chargeTimer_ = 0;
	chargeMode_ = 0;

	pTaskCircle_ = nullptr;

	{
		GET_INSTANCE(ResourceManager, resourceManager);

		auto textureBoss = resourceManager->GetResourceAs<TextureResource>("img/stage/dot_shinki.png");

		objBossAnimation_.SetTexture(textureBoss);
		objBossAnimation_.SetSourceRect(DxRectangle<int>(0, 0, 128, 128));
		objBossAnimation_.SetDestCenter();
		objBossAnimation_.UpdateVertexBuffer();
	}

	{
		auto sceneBG = (Stage_BackgroundScene1*)(parent_->GetParentManager()->GetScene(Scene::Background).get());
		pTaskBackground_ = std::dynamic_pointer_cast<Boss_SpellBackground>(sceneBG->GetBackground("ST1_BOSS"));
	}

	{
		auto music = ((Stage_MainScene*)parent_)->GetBGM();
		music->GetData()->Play();
		music->GetData()->SetVolumeRate(100);
	}

	{
		std::list<shared_ptr<Stage_EnemyPhase>> listPhase;

		listPhase.push_back(shared_ptr<Shinki_Non1>(new Shinki_Non1(parent_, this)));
		listPhase.push_back(shared_ptr<Shinki_Spell1>(new Shinki_Spell1(parent_, this)));
		listPhase.push_back(shared_ptr<Shinki_Spell2>(new Shinki_Spell2(parent_, this)));
		listPhase.push_back(shared_ptr<Shinki_DeadTask>(new Shinki_DeadTask(parent_, this)));

		InitializePhases(listPhase);
	}

	g_spellBonusWait->Init((Stage_MainScene*)parent_);
}

void EnemyBoss_Shinki::_RunAnimation() {
	constexpr int FRAME_I = 8;
	constexpr int FRAME_M = 7;

	static const int listSpriteIdle[] = { 0, 10, 20, 10, 0, 10, 20, 10, 0, 1, 11 };
	static const int listSpriteLeft[] = { 2, 2, 12, 12 };
	static const int listSpriteRight[] = { 22, 22, 32, 32 };
	static const std::vector<int> vecListSpriteCharge[] = {
		{ 4, 14, 24 },
		{ 5, 15, 25 },
	};
	static const int countSpriteIdle = sizeof(listSpriteIdle) / sizeof(int);
	static const int countSpriteLeft = sizeof(listSpriteLeft) / sizeof(int);
	static const int countSpriteRight = sizeof(listSpriteRight) / sizeof(int);
	static const int countVecSpriteCharge = sizeof(vecListSpriteCharge) / sizeof(std::vector<int>);

	int chargeMode = std::min<int>(chargeMode_, countVecSpriteCharge);

	double speedX = pattern_ ? pattern_->GetSpeedX() : 0;

	if (speedX < 0) {
		if (frameAnimLeft_ < countSpriteLeft * FRAME_M - 1)
			++frameAnimLeft_;
		frameAnimCharge_ = 0;
	}
	else if (frameAnimLeft_ > 0) {
		frameAnimLeft_ -= 2;
	}

	if (speedX > 0) {
		if (frameAnimRight_ < countSpriteRight * FRAME_M - 1)
			++frameAnimRight_;
		frameAnimCharge_ = 0;
	}
	else if (frameAnimRight_ > 0) {
		frameAnimRight_ -= 2;
	}

	if (chargeTimer_ > 0) {
		if (frameAnimCharge_ < vecListSpriteCharge[chargeMode].size() * FRAME_I - 1)
			++frameAnimCharge_;
		--chargeTimer_;
	}
	else if (frameAnimCharge_ > 0) {
		--frameAnimCharge_;
	}

	{
		int imgRender = 0;

		if (frameAnimLeft_ > 0) {
			int aFrame = (frameAnimLeft_ / FRAME_M) % countSpriteLeft;
			imgRender = listSpriteLeft[aFrame];
		}
		else if (frameAnimRight_ > 0) {
			int aFrame = (frameAnimRight_ / FRAME_M) % countSpriteRight;
			imgRender = listSpriteRight[aFrame];
		}
		else {		//Idle
			if (frameAnimCharge_ > 0) {
				const int* pSprites = vecListSpriteCharge[chargeMode].data();
				size_t maxCount = vecListSpriteCharge[chargeMode].size();
				imgRender = pSprites[(frameAnimCharge_ / FRAME_I) % maxCount];
			}
			else
				imgRender = listSpriteIdle[(frame_ / FRAME_I) % countSpriteIdle];
		}

		DxRectangle<int> frameRect = DxRectangle<int>::SetFromIndex(128, 128, imgRender, 10);
		objBossAnimation_.SetScroll(frameRect.left / 1024.0f, frameRect.top / 512.0f);
		//objSprite_.SetSourceRect(frameRect);
		//objSprite_.UpdateVertexBuffer();
	}

	objBossAnimation_.SetPosition(posX_, posY_ + sin(frame_ * GM_DTORA(2.37)) * 4, 1.0f);
}

void EnemyBoss_Shinki::Render(byte layer) {
	if (layer != LAYER_ENEMY) return;
	objBossAnimation_.Render();
}
void EnemyBoss_Shinki::Update() {
	if (frame_ == 60) {
		if (pTaskCircle_) pTaskCircle_->bEnd_ = true;
		pTaskCircle_ = shared_ptr<Boss_MagicCircle>(new Boss_MagicCircle(parent_, this));
		parent_->AddTask(pTaskCircle_);

		{
			auto stageUI = parent_->GetParentManager()->GetScene(Scene::StageUI).get();
			if (pTaskLifebar_) pTaskLifebar_->frameEnd_ = 0;
			pTaskLifebar_ = shared_ptr<Enemy_LifeIndicatorBoss>(new Enemy_LifeIndicatorBoss(stageUI, this));
			stageUI->AddTask(pTaskLifebar_);
		}
	}

	if (IsAllPhasesFinished()) {
		if (pTaskCircle_) pTaskCircle_->bEnd_ = true;
		if (pTaskLifebar_) pTaskLifebar_->frameEnd_ = 0;
		frameEnd_ = 0;

		class WaitClose : public TaskBase {
		public:
			WaitClose(Scene* parent) : TaskBase(parent) {
				frameEnd_ = 300;
			}
			~WaitClose() {
				((Stage_MainScene*)parent_)->CloseStage();
			}
			virtual void Update() {
				++frame_;
			}
		};
		parent_->AddTask(new WaitClose(parent_));

		return;
	}

	_Move();
	_RunAnimation();

	//Update intersection
	{
		auto intersectionManager = ((Stage_MainScene*)parent_)->GetIntersectionManager();
		{
			auto pTarget = new Stage_IntersectionTarget_Circle();
			pTarget->SetTargetType(Stage_IntersectionTarget::TypeTarget::EnemyToPlayer);
			pTarget->SetParent(pOwnRefWeak_);
			pTarget->SetCircle(DxCircle<float>(posX_, posY_, 24));
			pTarget->SetIntersectionSpace();
			intersectionManager->AddTarget(shared_ptr<Stage_IntersectionTarget>(pTarget));
		}
		{
			auto pTarget = new Stage_IntersectionTarget_Circle();
			pTarget->SetTargetType(Stage_IntersectionTarget::TypeTarget::EnemyToPlayerShot);
			pTarget->SetParent(pOwnRefWeak_);
			pTarget->SetCircle(DxCircle<float>(posX_, posY_, 48));
			pTarget->SetIntersectionSpace();
			intersectionManager->AddTarget(shared_ptr<Stage_IntersectionTarget>(pTarget));
		}
	}

	Stage_EnemyTask_Scripted::Update();
	++frame_;
}

void EnemyBoss_Shinki::SetSpellBackground(bool b) {
	pTaskBackground_->SetVisible(b);
}

//*******************************************************************
//Boss_MagicCircle
//*******************************************************************
Boss_MagicCircle::Boss_MagicCircle(Scene* parent, EnemyBoss_Shinki* objBoss) : TaskBase(parent) {
	objBoss_ = objBoss;

	bEnd_ = false;

	ZeroMemory(tAngle_, sizeof(tAngle_));
	ZeroMemory(tScale_, sizeof(tScale_));
	ZeroMemory(tAlpha_, sizeof(tAlpha_));

	{
		GET_INSTANCE(ResourceManager, resourceManager);

		objCircle_.SetTexture(resourceManager->GetResourceAs<TextureResource>("img/stage/eff_magicsquare.png"));
		objCircle_.SetSourceRectNormalized(DxRectangle<float>(0, 0, 1, 1));
		objCircle_.SetDestCenter();
		objCircle_.UpdateVertexBuffer();

		objCircle_.SetBlendType(BlendMode::Add);
	}
}
void Boss_MagicCircle::Render(byte layer) {
	if (layer != LAYER_ENEMY - 1) return;
	objCircle_.Render();
}
void Boss_MagicCircle::Update() {
	if (!bEnd_) {
		if (frame_ < 128) {
			double rate = frame_ / 127.0;
			tAngle_[1] = Math::Lerp::Decelerate<double>(6.3, 1, rate);
			tScale_[0] = Math::Lerp::Smooth<double>(0, 1.2, rate);
			tAlpha_[0] = Math::Lerp::Linear<double>(0, 1, rate);
		}
		else {
			tAngle_[1] = 1;
			tScale_[0] = 1.2;
			tAlpha_[0] = 1;
		}
	}
	else {
		if (frameEnd_ == UINT_MAX) {
			frameEnd_ = 40;
			frame_ = 0;
			tScale_[1] = tScale_[0];
			tAlpha_[1] = tAlpha_[0];
		}

		double rate = frame_ / 39.0;
		tScale_[0] = Math::Lerp::Accelerate<double>(tScale_[1], 0, rate);
		tAlpha_[0] = Math::Lerp::Smooth<double>(tAlpha_[1], 0, rate);
	}

	{
		double sFrame = tAngle_[0];
		double angleZ = sFrame * GM_DTORA(1.12);
		double angleX = GM_DTORA(36) * cos(sFrame * GM_DTORA(0.842));
		double angleY = sin(sFrame * GM_DTORA(0.711));

		objCircle_.SetAngle(angleX, GM_DTORA(33) * angleY * angleY, angleZ);
	}

	if (!bEnd_)
		objCircle_.SetPosition(objBoss_->GetMovePosition());
	objCircle_.SetScale(tScale_[0], tScale_[0], 1.0f);
	objCircle_.SetAlpha(tAlpha_[0] * 128);

	tAngle_[0] += tAngle_[1];
	++frame_;
}

//*******************************************************************
//Enemy_LifeIndicatorBoss
//*******************************************************************
Enemy_LifeIndicatorBoss::Enemy_LifeIndicatorBoss(Scene* parent, Stage_EnemyTask_Scripted* objEnemy) : TaskBase(parent) {
	objEnemy_ = objEnemy;

	prevScale_ = 0;

	{
		std::vector<VertexTLX> vertex;
		vertex.resize(6);

		D3DCOLOR colorCen = D3DCOLOR_XRGB(255, 255, 255);
		D3DCOLOR colorEnd = D3DCOLOR_XRGB(192, 192, 192);

		vertex[0] = VertexTLX(D3DXVECTOR3(-192, 0, 1), D3DXVECTOR2(), colorEnd);
		vertex[1] = VertexTLX(D3DXVECTOR3(-192, BAR_WD, 1), D3DXVECTOR2(), colorEnd);

		vertex[2] = VertexTLX(D3DXVECTOR3(0, 0, 1), D3DXVECTOR2(), colorCen);
		vertex[3] = VertexTLX(D3DXVECTOR3(0, BAR_WD, 1), D3DXVECTOR2(), colorCen);

		vertex[4] = VertexTLX(D3DXVECTOR3(192, 0, 1), D3DXVECTOR2(), colorEnd);
		vertex[5] = VertexTLX(D3DXVECTOR3(192, BAR_WD, 1), D3DXVECTOR2(), colorEnd);

		objBar_.SetPrimitiveType(D3DPT_TRIANGLESTRIP);
		objBar_.SetArrayVertex(vertex);
	}
}

void Enemy_LifeIndicatorBoss::Render(byte layer) {
	if (layer != 1) return;

	auto stage = (Stage_MainScene*)(parent_->GetParentManager()->GetScene(Scene::Stage).get());
	auto objPlayer = stage->GetPlayer();

	objBar_.SetPosition(320, 16, 1);
	objBar_.SetAlpha(objPlayer->posY_ < 16 + BAR_WD + 24 ? 96 : 255);
	objBar_.Render();
	objBar_.SetPosition(320, 464 + 2, 1);
	objBar_.SetAlpha(255);
	objBar_.Render();
}
void Enemy_LifeIndicatorBoss::Update() {
	auto stage = (Stage_MainScene*)(parent_->GetParentManager()->GetScene(Scene::Stage).get());

	double targetScale = 0;
	if (auto pPhase = objEnemy_->GetCurrentPhase()) {
		targetScale = objEnemy_->life_ / pPhase->lifeMax_;
	}

	prevScale_ = prevScale_ * 0.9 + targetScale * 0.1;
	objBar_.SetScaleX(std::clamp<double>(prevScale_, 0, 1));

	{
		D3DXVECTOR3 color;
		if (prevScale_ > 0.5) {
			double rate = (prevScale_ - 0.5) * 2;
			color.x = Math::Lerp::Smooth(248, 78, rate);
			color.y = Math::Lerp::Smooth(186, 255, rate);
			color.z = Math::Lerp::Smooth(44, 121, rate);
		}
		else {
			double rate = prevScale_ * 2;
			color.x = Math::Lerp::Smooth(255, 248, rate);
			color.y = Math::Lerp::Smooth(4, 186, rate);
			color.z = Math::Lerp::Smooth(4, 44, rate);
		}
		objBar_.SetColor(color / 255.0f);
	}
}

//----------------------------------------------------------------------------------------------------------

//*******************************************************************
//Boss_SpellBackground
//*******************************************************************
Boss_SpellBackground::Boss_SpellBackground(Scene* parent) : TaskBase(parent) {
	GET_INSTANCE(ResourceManager, resourceManager);

	bVisible_ = false;
	tAlpha_ = 0;

	{
		objBack_.SetTexture(
			resourceManager->GetResourceAs<TextureResource>("img/stage/background/bg_spell0.png"));
		objBack_.SetSourceRect(DxRectangle<int>(0, 0, 512, 512));
		objBack_.SetDestCenter();
		objBack_.UpdateVertexBuffer();

		objBack_.SetColor(170, 170, 170);
		objBack_.SetPosition(320, 240, 1);
		objBack_.SetScale(1.25, 1.25, 1);	//640 / 512
	}

	{
		objFront_.SetTexture(
			resourceManager->GetResourceAs<TextureResource>("img/stage/background/bg_spell1.png"));
		objFront_.SetSourceRect(DxRectangle<int>(0, 0, 512, 512));
		objFront_.SetDestCenter();
		objFront_.UpdateVertexBuffer();

		objFront_.SetPosition(320, 240, 1);
	}
}

void Boss_SpellBackground::Render(byte layer) {
	if (layer != 2) return;
	objBack_.SetAlpha(255 * tAlpha_);
	objBack_.Render();

	{
		objFront_.SetBlendType(BlendMode::Subtract);
		objFront_.SetAlpha(255 * tAlpha_);
		objFront_.SetScale(1.2, 1.2, 1);
		objFront_.SetAngleZ(frame_ * GM_DTORA(0.2581));
		objFront_.Render();
	}
	{
		double tmp_sc = 1 + sin(frame_ * GM_DTORA(0.68)) * 0.08;

		objFront_.SetBlendType(BlendMode::Alpha);
		objFront_.SetAlpha(224 * tAlpha_);
		objFront_.SetScale(tmp_sc, tmp_sc, 1);
		objFront_.SetAngleZ(0);
		objFront_.Render();
	}
}
void Boss_SpellBackground::Update() {
	if (bVisible_)
		IncUntil(tAlpha_, 1 / 20.0, 1.0);
	else
		DecUntil(tAlpha_, 1 / 30.0, 0.0);

	++frame_;
}