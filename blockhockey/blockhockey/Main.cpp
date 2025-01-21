# include <Siv3D.hpp>
# include <HamFramework.hpp>

struct GameData //遷移時に保存しておく内容
{
	bool isWin = true;
};

using App = SceneManager<String, GameData>;

class Title : public App::Scene //タイトルシーン
{
public:
	const Array<String> texts =
	{
		U"GameStart",
		U"Exit",
	};
	const Array<Rect> buttons =
	{
		Rect{250, 300, 300, 80},
		Rect{250, 400, 300, 80},
	};
	int selected = -1;
	const Font font = Font{ 40 };
	const Texture texture{ U"images/title.png" };

	Title(const InitData& init)
		: IScene{ init }
	{
		//Print << buttons.size();
	}

	void update() override  //ボタン遷移
	{
		int i = 0;
		selected = -1;
		for (i = 0; i < buttons.size(); ++i)
		{
			if (buttons[i].mouseOver())
			{
				Cursor::RequestStyle(CursorStyle::Hand);
				selected = i;
			}
			if (buttons[i].leftClicked())
			{
				if (i) System::Exit();
				else changeScene(U"Game");
			}
		}
	}

	void draw() const override
	{
		texture.resized(Scene::Width()).draw();
		int i;
		for (i = 0; i < buttons.size(); ++i) {
			const bool isSelected = (selected == i);
			const String text = texts[i];
			const Rect button = buttons[i];
			if (isSelected) {
				button.draw(ColorF{ 0.3, 0.7, 1.0 });
				font(text).drawAt(40, (button.x + button.w / 2), (button.y + button.h / 2));
			}
			else {
				button.draw(ColorF{ 0.4, 0.8, 1.0 });
				font(text).drawAt(40, (button.x + button.w / 2), (button.y + button.h / 2));
			}
		}
	}
};

class Game : public App::Scene //ゲーム本体部分
{
public:
	// ブロックのサイズ
	Size brickSize{ 40, 20 };

	//ボールサイズ
	int ballSize;

	// ボールの速さ
	const double speed = 480.0;

	// ボールの速度
	Array<Vec2> ballsVelocity;

	// ボール
	Array<Circle> balls;

	// ブロックの配列
	Array<Rect> bricks;

	//ブロックが崩れているか
	Array<bool> bricksFlg;

	// パドル
	Array<Circle> paddles;

	//初期ボール位置
	Array<Circle> ballsInit;

	//相手パドル速さ
	double maxSpeed = 100.0;

	//設定読み込み
	CSV csv{ U"config.csv" };

	//パドル可動範囲
	int paddleRange[2][4] = { {0,0,0,0}, {0,0,0,0} };

	//ボール待機時間管理
	double timer[2] = { 0.0, 0.0 };

	//左右描画範囲
	int xRange[2] = { 200, 600 };

	//ボール待機時間
	double waitTime;

	//背景
	const Texture texture{ U"images/game.png" };

	//落とした時の挙動
	bool isapp = 1;

	//落とした時に変化するブロックの数
	int change_num = 0;

	Game(const InitData& init)
		: IScene{ init }
	{
		int i;
		if (not csv)
		{
			throw Error{ U"Failed to load `config.csv`" };
		}

		//パラメータ初期化
		ballsVelocity << Vec2{ 0, -speed };
		ballsVelocity << Vec2{ 0, speed };
		balls << Circle{ 400, 300 + Parse<int>(csv[8][1]), 8 };
		balls << Circle{ 400, 300 - Parse<int>(csv[8][1]), 8 };

		ballSize = Parse<int>(csv[4][1]);

		isapp = Parse<int>(csv[3][1]) ? true : false;

		change_num = Parse<int>(csv[2][1]);

		csv[12][1] = Format(400);
		csv[12][2] = Format(600);
		csv[13][1] = Format(400);
		csv[13][2] = Format(50);
		csv[13][3] = Format(10);

		paddleRange[0][0] = Scene::Width() / 2 - Parse<int>(csv[13][1]) / 2;
		paddleRange[0][1] = Scene::Width() / 2 + Parse<int>(csv[13][1]) / 2;
		paddleRange[0][2] = Scene::Height() - Parse<int>(csv[13][2]);
		paddleRange[0][3] = Scene::Height() - Parse<int>(csv[13][3]);

		paddleRange[1][0] = Scene::Width() / 2 - Parse<int>(csv[13][1]) / 2;
		paddleRange[1][1] = Scene::Width() / 2 + Parse<int>(csv[13][1]) / 2;
		paddleRange[1][2] = Parse<int>(csv[13][3]);
		paddleRange[1][3] = Parse<int>(csv[13][2]);

		paddles << Circle{
			Arg::center(
				Clamp(Cursor::Pos().x, paddleRange[0][0], paddleRange[0][1]),
				Clamp(Cursor::Pos().y, paddleRange[0][2], paddleRange[0][3])
			),
				ballSize
		};

		paddles << Circle{
			Arg::center(Scene::Width() / 2, 10 + ballSize), ballSize
		};

		for (i = 0; i < 2; ++i) ballsInit << balls[i];

		waitTime = (double)(Parse<int>(csv[9][1]));

		if (Parse<int>(csv[6][1]) * Parse<int>(csv[5][1]) > 400) csv[6][1] = Format(400 / Parse<int>(csv[5][1]));
		if (Parse<int>(csv[6][2]) * Parse<int>(csv[5][2]) + Parse<int>(csv[7][1]) > Parse<int>(csv[8][1]))
			csv[6][2] = Format((Parse<int>(csv[8][1]) - Parse<int>(csv[7][1])) / Parse<int>(csv[5][2]));

		// ブロックを配列に追加する

		brickSize = Size{ Parse<int>(csv[5][1]),  Parse<int>(csv[5][2]) };
		Size initPos{
			(Scene::Width() - Parse<int>(csv[5][1]) * Parse<int>(csv[6][1])) / 2,
			Scene::Height() / 2 + Parse<int>(csv[7][1])
		};
		for (auto p : step(Size{ Parse<int>(csv[6][1]), Parse<int>(csv[6][2]) }))
		{
			bricks << Rect{ (initPos.x + p.x * brickSize.x), (initPos.y + p.y * brickSize.y), brickSize };
			bricksFlg << true;
		}

		initPos.y = Scene::Height() / 2.0 - Parse<int>(csv[7][1]) - brickSize.y;
		for (auto p : step(Size{ Parse<int>(csv[6][1]), Parse<int>(csv[6][2]) }))
		{
			bricks << Rect{ (initPos.x + p.x * brickSize.x), (initPos.y - p.y * brickSize.y), brickSize };
			bricksFlg << true;
		}

	}

	void update() override
	{
		int i, j;

		//パドル移動
		Vec2 prevPos[2] = { paddles[0].center, paddles[1].center };
		paddles[0] = Circle{
			Arg::center(
				Clamp(Cursor::Pos().x, paddleRange[0][0], paddleRange[0][1]),
				Clamp(Cursor::Pos().y, paddleRange[0][2], paddleRange[0][3])
			),
			ballSize
		};

		//相手パドル移動
		int min = 0;
		if (balls[0].y > balls[1].y) min = 1;
		if (balls[min].x > paddles[1].x) paddles[1].x = Math::Min(paddles[1].x + Scene::DeltaTime() * maxSpeed, balls[min].x);
		else paddles[1].x = Math::Max(paddles[1].x - Scene::DeltaTime() * maxSpeed, balls[min].x); //自分に近い側のボールの動きに合わせる
		if (ballsVelocity[min].y < 0 && balls[min].y < -ballsVelocity[min].y * 5 * Scene::DeltaTime() + paddleRange[1][2]) paddles[1].y = Math::Min(paddles[1].y + maxSpeed * Scene::DeltaTime(), paddleRange[1][3]);
		else paddles[1].y = Math::Max(paddles[1].y - maxSpeed * Scene::DeltaTime(), paddleRange[1][2]); //y方向にも動かす

		//各ボールの処理
		for (i = 0; i < 2; ++i)
		{
			Circle ball = balls[i];
			Vec2 ballVelocity = ballsVelocity[i];
			if (!ballVelocity.x && !ballVelocity.y) { //ボール待機中
				timer[i] += Scene::DeltaTime();
				if (timer[i] > waitTime) ballsVelocity[i].y = i ? speed : -speed;
			}
			else if (Math::Abs(ballsVelocity[i].y) < 2) ballsVelocity[i].y = ballsVelocity[i].y < 0.0 ? -2.0 : 2.0;
			// ボールを移動
			balls[i].moveBy(ballVelocity * Scene::DeltaTime());

			ball = balls[i];

			// ブロックを順にチェック
			for (auto it = bricks.begin(); it != bricks.end(); ++it)
			{
				if (bricksFlg[it - bricks.begin()])
				{
					// ブロックとボールが交差していたら
					if (it->intersects(ball))
					{
						// ボールの向きを反転する
						(it->bottom().intersects(ball) || it->top().intersects(ball)
							? ballsVelocity[i].y : ballsVelocity[i].x) *= -1;

						ballVelocity = ballsVelocity[i];

						// ブロックのフラグを折っておく
						bricksFlg[it - bricks.begin()] = false;

						// これ以上チェックしない
						break;
					}
				}
			}

			// 落としたら再配置&ブロック処理
			if ((ball.y < 0 && ballVelocity.y < 0) || (600 < ball.y && ballVelocity.y > 0))
			{
				//ブロック処理の方法
				int check = isapp ? 1 : 0;
				if (ball.y < 0) check = (check + 1) & 1;
				check *= bricks.size() / 2;

				//ボール待機状態に
				ball = balls[i] = ballsInit[i];
				ballVelocity = ballsVelocity[i] = Vec2{ 0,0 };
				timer[i] = 0.0;

				//ブロックの処理
				Array<int> vec_rand;
				for (j = check; j < check + bricks.size() / 2; ++j) {
					if (isapp) {
						if (bricksFlg[j]) vec_rand << j;
					}
					else {
						if (!bricksFlg[j]) vec_rand << j;
					}
				}
				if (vec_rand.size()) { //条件に合うブロックからランダムに選ぶ
					for (j = 0; j < 100; ++j) {
						int r1 = rand() % vec_rand.size();
						int r2 = rand() % vec_rand.size();
						int t = vec_rand[r1];
						vec_rand[r1] = vec_rand[r2];
						vec_rand[r2] = t;
					}
				}
				for (j = 0; j < Min<int>(change_num, vec_rand.size()); ++j) bricksFlg[vec_rand[j]] = !bricksFlg[vec_rand[j]];
			}

			// 左右の壁にぶつかったらはね返る
			if ((ball.x < xRange[0] && ballVelocity.x < 0)
				|| (xRange[1] < ball.x && 0 < ballVelocity.x))
			{
				ballVelocity.x = ballsVelocity[i].x *= -1;
			}

			// パドルにあたったらはね返る
			for (j = 0; j < 2; ++j)
			{
				Circle paddle = paddles[j];
				if (((0 < ballVelocity.y) ^ j) && paddle.intersects(ball)) //こちら側に向かっているときのみ処理する
				{
					double nowSpeed = ballVelocity.length();

					//前処理部分、法線ベクトル、接線ベクトルを求める
					Vec2 direct = Vec2{ paddle.center.x - ball.center.x,  paddle.center.y - ball.center.y }.normalize(),
					addVelocity = direct * direct.dot(paddle.center - prevPos[j]) / Scene::DeltaTime(),
					directPerp = direct.rotated90(),
					nowVelH = direct * ballVelocity.dot(direct),
					nowVelP = direct * ballVelocity.dot(directPerp);

					// はね返す
					ballsVelocity[i] = nowVelP + nowVelH * -1 + addVelocity;
					ballsVelocity[i].x += Random() - 0.5;
					if (Math::Abs(ballsVelocity[i].y) < 10) ballsVelocity[i].y = ballsVelocity[i].y > 0 ? 10 : -10;
					ballVelocity = ballsVelocity[i];
				}
			}
		}

		//ゲーム終了判定
		bool flg = true;
		for (i = 0; i < bricks.size() / 2; ++i) if (bricksFlg[i]) {
			flg = false;
			break;
		}
		if (flg) { //勝利
			getData().isWin = true;
			changeScene(U"End");
		}
		flg = true;
		for (i = bricks.size() / 2; i < bricks.size(); ++i) if (bricksFlg[i]) {
			flg = false;
			break;
		}
		if (flg) { //敗北
			getData().isWin = false;
			changeScene(U"End");
		}
	}

	void draw() const override
	{
		texture.resized(600).drawAt(400, 300);
		int i;
		// すべてのブロックを描画する
		for (i = 0; i < bricks.size(); ++i) if (bricksFlg[i])
		{
			bricks[i].stretched(-1).draw(ColorF(0.0, 1.0, 0.0));
		}
		for (i = 0; i < 2; ++i)
		{
			// ボールを描く
			balls[i].draw(i ? ColorF(1.0, 0.0, 0.0) : ColorF(0.0, 0.0, 1.0));

			// パドルを描く
			paddles[i].draw();
		}
	}
};

class End : public App::Scene //終了画面
{
public:
	const Array<String> texts =
	{
		U"Retry",
		U"Exit",
	};
	const Array<Rect> buttons =
	{
		Rect{250, 300, 300, 80},
		Rect{250, 400, 300, 80},
	};
	int selected = -1;
	const Font font = Font{ 40 };

	End(const InitData& init)
		: IScene{ init }
	{
		//Print << buttons.size();
	}

	void update() override
	{
		int i = 0;
		selected = -1;
		for (i = 0; i < buttons.size(); ++i)
		{
			if (buttons[i].mouseOver())
			{
				Cursor::RequestStyle(CursorStyle::Hand);
				selected = i;
			}
			if (buttons[i].leftClicked())
			{
				if (i) changeScene(U"Title");
				else changeScene(U"Exit");
			}
		}
	}

	void draw() const override
	{
		int i;
		for (i = 0; i < buttons.size(); ++i) {
			const bool isSelected = (selected == i);
			const String text = texts[i];
			const Rect button = buttons[i];
			if (isSelected) {
				button.draw(ColorF{ 0.3, 0.7, 1.0 });
				font(text).drawAt(40, (button.x + button.w / 2), (button.y + button.h / 2));
			}
			else {
				button.draw(ColorF{ 0.4, 0.8, 1.0 });
				font(text).drawAt(40, (button.x + button.w / 2), (button.y + button.h / 2));
			}
		}
		if (getData().isWin) font(U"Win").drawAt(40, 400, 200);
		else font(U"Lose").drawAt(40, 400, 200);
	}
};

void Main()
{
	App manager;
	manager.add<Title>(U"Title");
	manager.add<Game>(U"Game");
	manager.add<End>(U"End");

	while (System::Update())
	{
		if (!manager.update())
		{
			break;
		}
	}
}
