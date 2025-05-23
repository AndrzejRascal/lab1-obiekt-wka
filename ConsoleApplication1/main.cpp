﻿#include <vector>
#include <algorithm>
#include <functional> 
#include <memory>
#include <cstdlib>
#include <cmath>
#include <ctime>

#include <raylib.h>
#include <raymath.h>

// --- UTILS ---
namespace Utils {
	inline static float RandomFloat(float min, float max) {
		return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
	}
}

// --- TRANSFORM, PHYSICS, LIFETIME, RENDERABLE ---
struct TransformA {
	Vector2 position{};
	float rotation{};
};

struct Physics {
	Vector2 velocity{};
	float rotationSpeed{};
};

struct Renderable {
	enum Size { SMALL = 1, MEDIUM = 2, LARGE = 4 } size = SMALL;
};

// --- RENDERER ---
class Renderer {
public:
	static Renderer& Instance() {
		static Renderer inst;
		return inst;
	}

	void Init(int w, int h, const char* title) {
		InitWindow(w, h, title);
		SetTargetFPS(60);
		screenW = w;
		screenH = h;
	}

	void Begin() {
		BeginDrawing();
		ClearBackground(BLACK);
	}

	void End() {
		EndDrawing();
	}

	void DrawPoly(const Vector2& pos, int sides, float radius, float rot) {
		DrawPolyLines(pos, sides, radius, rot, WHITE);
	}

	int Width() const {
		return screenW;
	}

	int Height() const {
		return screenH;
	}

private:
	Renderer() = default;

	int screenW{};
	int screenH{};
};

// --- ASTEROID HIERARCHY ---

class Asteroid {
public:
	Asteroid(int screenW, int screenH) {
		init(screenW, screenH);
	}
	virtual ~Asteroid() = default;

	bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));
		transform.rotation += physics.rotationSpeed * dt;
		if (transform.position.x < -GetRadius() || transform.position.x > Renderer::Instance().Width() + GetRadius() ||
			transform.position.y < -GetRadius() || transform.position.y > Renderer::Instance().Height() + GetRadius())
			return false;
		return true;
	}
	virtual void Draw() const = 0;

	Vector2 GetPosition() const {
		return transform.position;
	}

	virtual float GetRadius() const {
		return 16.f * (float)render.size;
	}

	int GetDamage() const {
		return baseDamage * static_cast<int>(render.size);
	}

	int GetSize() const {
		return static_cast<int>(render.size);
	}

protected:
	void init(int screenW, int screenH) {
		// Choose size
		render.size = static_cast<Renderable::Size>(1 << GetRandomValue(0, 2));

		// Spawn at random edge
		switch (GetRandomValue(0, 3)) {
		case 0:
			transform.position = { Utils::RandomFloat(0, screenW), -GetRadius() };
			break;
		case 1:
			transform.position = { screenW + GetRadius(), Utils::RandomFloat(0, screenH) };
			break;
		case 2:
			transform.position = { Utils::RandomFloat(0, screenW), screenH + GetRadius() };
			break;
		default:
			transform.position = { -GetRadius(), Utils::RandomFloat(0, screenH) };
			break;
		}

		// Aim towards center with jitter
		float maxOff = fminf(screenW, screenH) * 0.1f;
		float ang = Utils::RandomFloat(0, 2 * PI);
		float rad = Utils::RandomFloat(0, maxOff);
		Vector2 center = {
										 screenW * 0.5f + cosf(ang) * rad,
										 screenH * 0.5f + sinf(ang) * rad
		};

		Vector2 dir = Vector2Normalize(Vector2Subtract(center, transform.position));
		physics.velocity = Vector2Scale(dir, Utils::RandomFloat(SPEED_MIN, SPEED_MAX));
		physics.rotationSpeed = Utils::RandomFloat(ROT_MIN, ROT_MAX);

		transform.rotation = Utils::RandomFloat(0, 360);
	}

	TransformA transform;
	Physics    physics;
	Renderable render;

	int baseDamage = 0;
	static constexpr float LIFE = 10.f;
	static constexpr float SPEED_MIN = 125.f;
	static constexpr float SPEED_MAX = 250.f;
	static constexpr float ROT_MIN = 50.f;
	static constexpr float ROT_MAX = 240.f;
};

class TriangleAsteroid : public Asteroid {
public:
	TriangleAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 5; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 3, GetRadius(), transform.rotation);
	}
};
class SquareAsteroid : public Asteroid {
public:
	SquareAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 10; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 4, GetRadius(), transform.rotation);
	}
};
class PentagonAsteroid : public Asteroid {
public:
	PentagonAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 15; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 5, GetRadius(), transform.rotation);
	}
};

// Shape selector
enum class AsteroidShape { TRIANGLE = 3, SQUARE = 4, PENTAGON = 5, RANDOM = 0 };

// Factory
static inline std::unique_ptr<Asteroid> MakeAsteroid(int w, int h, AsteroidShape shape) {
	switch (shape) {
	case AsteroidShape::TRIANGLE:
		return std::make_unique<TriangleAsteroid>(w, h);
	case AsteroidShape::SQUARE:
		return std::make_unique<SquareAsteroid>(w, h);
	case AsteroidShape::PENTAGON:
		return std::make_unique<PentagonAsteroid>(w, h);
	default: {
		return MakeAsteroid(w, h, static_cast<AsteroidShape>(3 + GetRandomValue(0, 2)));
	}
	}
}
class BigAsteroid : public Asteroid {
public:
	BigAsteroid(int w, int h) : Asteroid(w, h) {
		baseDamage = 30;
		hp = 1000; // <- teraz wymaga 20 trafień
		render.size = Renderable::LARGE;
		// Ustaw pozycję na losowej krawędzi
		transform.position = { Utils::RandomFloat(0, w), -GetRadius() };
		// Kierunek do środka ekranu
		Vector2 center = { w * 0.5f, h * 0.5f };
		Vector2 dir = Vector2Normalize(Vector2Subtract(center, transform.position));
		physics.velocity = Vector2Scale(dir, 100.0f);
		physics.rotationSpeed = Utils::RandomFloat(20.f, 60.f);
		transform.rotation = Utils::RandomFloat(0, 360);
	}
	void Draw() const override {
		DrawCircleLinesV(transform.position, GetRadius(), RED);
		Renderer::Instance().DrawPoly(transform.position, 8, GetRadius(), transform.rotation);
		DrawText(TextFormat("%d", hp), (int)transform.position.x - 10, (int)transform.position.y - 10, 20, RED);
	}
	float GetRadius() const override {
		return 64.f; // Duży promień
	}
	int hp; // <- zmienione z 10 na 20
};

// --- PROJECTILE HIERARCHY ---
enum class WeaponType { LASER, BULLET, ROCKET, PLASMA, SPECIAL, COUNT };
class Projectile {
public:
	Projectile(Vector2 pos, Vector2 vel, int dmg, WeaponType wt)
	{
		transform.position = pos;
		physics.velocity = vel;
		baseDamage = dmg;
		type = wt;
	}
	bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));

		if (transform.position.x < 0 ||
			transform.position.x > Renderer::Instance().Width() ||
			transform.position.y < 0 ||
			transform.position.y > Renderer::Instance().Height())
		{
			return true;
		}
		return false;
	}
	void Draw() const {
		switch (type) {
		case WeaponType::SPECIAL:
			DrawCircleV(transform.position, 200.f, GOLD);
			DrawCircleV(transform.position, 250.f, RED);
			break;
		case WeaponType::BULLET:
			DrawCircleV(transform.position, 5.f, WHITE);
			break;
		case WeaponType::LASER:
		{
			static constexpr float LASER_LENGTH = 30.f;
			Rectangle lr = { transform.position.x - 2.f, transform.position.y - LASER_LENGTH, 4.f, LASER_LENGTH };
			DrawRectangleRec(lr, RED);
		}
		break;
		case WeaponType::ROCKET:
			DrawCircleV(transform.position, 8.f, ORANGE);
			DrawCircleV({ transform.position.x, transform.position.y + 14.f }, 30.f, YELLOW);
			break;
		case WeaponType::PLASMA:
			DrawCircleV(transform.position, 3.f, SKYBLUE);
			DrawCircleV(transform.position, 1.f, VIOLET);
			break;
		default:
			break;
		}
	}
	Vector2 GetPosition() const {
		return transform.position;
	}

	float GetRadius() const {
		if (type == WeaponType::SPECIAL) return 18.f;
		return (type == WeaponType::BULLET) ? 5.f : 2.f;
	}

	int GetDamage() const {
		return baseDamage;
	}

private:
	TransformA transform;
	Physics    physics;
	int        baseDamage;
	WeaponType type;
};

inline static Projectile MakeProjectile(WeaponType wt,
	const Vector2 pos,
	float speed)
{
	Vector2 vel{ 0, -speed };
	if (wt == WeaponType::LASER) {
		return Projectile(pos, vel, 20, wt);
	}
	else {
		return Projectile(pos, vel, 10, wt);
	}
}

// --- SHIP HIERARCHY ---
class Ship {
public:
	void SetHP(int value) { hp = value; }
	Ship(int screenW, int screenH) {
		transform.position = {
												 screenW * 0.5f,
												 screenH * 0.5f
		};
		hp = 100;
		speed = 250.f;
		alive = true;

		// per-weapon fire rate & spacing
		fireRateLaser = 18.f; // shots/sec
		fireRateBullet = 22.f;
		spacingLaser = 40.f; // px between lasers
		spacingBullet = 20.f;
	}
	virtual ~Ship() = default;
	virtual void Update(float dt) = 0;
	virtual void Draw() const = 0;

	void TakeDamage(int dmg) {
		if (!alive) return;
		hp -= dmg;
		if (hp <= 0) alive = false;
	}

	bool IsAlive() const {
		return alive;
	}

	Vector2 GetPosition() const {
		return transform.position;
	}

	virtual float GetRadius() const = 0;

	int GetHP() const {
		return hp;
	}

	float GetFireRate(WeaponType wt) const {
		return (wt == WeaponType::LASER) ? fireRateLaser : fireRateBullet;
	}

	float GetSpacing(WeaponType wt) const {
		return (wt == WeaponType::LASER) ? spacingLaser : spacingBullet;
	}

protected:
	TransformA transform;
	int        hp;
	float      speed;
	bool       alive;
	float      fireRateLaser;
	float      fireRateBullet;
	float      spacingLaser;
	float      spacingBullet;
};

class PlayerShip :public Ship {
public:
	PlayerShip(int w, int h) : Ship(w, h) {
		texture = LoadTexture("spaceship1.png");
		GenTextureMipmaps(&texture);                                                        // Generate GPU mipmaps for a texture
		SetTextureFilter(texture, 2);
		scale = 0.25f;
	}
	~PlayerShip() {
		UnloadTexture(texture);
	}

	void Update(float dt) override {
		if (alive) {
			if (IsKeyDown(KEY_W)) transform.position.y -= speed * dt;
			if (IsKeyDown(KEY_S)) transform.position.y += speed * dt;
			if (IsKeyDown(KEY_A)) transform.position.x -= speed * dt;
			if (IsKeyDown(KEY_D)) transform.position.x += speed * dt;
		}
		else {
			transform.position.y += speed * dt;
		}
	}

	void Draw() const override {
		if (!alive && fmodf(GetTime(), 0.4f) > 0.2f) return;
		Vector2 dstPos = {
										 transform.position.x - (texture.width * scale) * 0.5f,
										 transform.position.y - (texture.height * scale) * 0.5f
		};
		DrawTextureEx(texture, dstPos, 0.0f, scale, WHITE);
	}

	float GetRadius() const override {
		return (texture.width * scale) * 0.5f;
	}

private:
	Texture2D texture;
	float     scale;
};

// --- APPLICATION ---
class Application {
public:
	static Application& Instance() {
		static Application inst;
		return inst;
	}

	void Run() {
		srand(static_cast<unsigned>(time(nullptr)));
		Renderer::Instance().Init(C_WIDTH, C_HEIGHT, "Asteroids OOP");

		auto player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);

		float spawnTimer = 0.f;
		float spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
		WeaponType currentWeapon = WeaponType::LASER;
		float shotTimer = 0.f;

		enum class ShootDir { UP, RIGHT, DOWN, LEFT };
		ShootDir shootDir = ShootDir::UP;

		downloadTexture = LoadTexture("download.jpg");

		while (!WindowShouldClose()) {
			float dt = GetFrameTime();
			spawnTimer += dt;
			if (gameEnded) {
				Renderer::Instance().Begin();
				int x = (C_WIDTH - downloadTexture.width) / 2;
				int y = (C_HEIGHT - downloadTexture.height) / 2;
				DrawTexture(downloadTexture, x, y, WHITE);
				Renderer::Instance().End();
				continue; // pomija resztę pętli gry
			}

			// Update player
			player->Update(dt);

			// Restart logic
			if (!player->IsAlive() && IsKeyPressed(KEY_R)) {
				player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);
				asteroids.clear();
				projectiles.clear();
				spawnTimer = 0.f;
				spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
			}
			// Asteroid shape switch
			if (IsKeyPressed(KEY_ONE)) {
				currentShape = AsteroidShape::TRIANGLE;
			}
			if (IsKeyPressed(KEY_TWO)) {
				currentShape = AsteroidShape::SQUARE;
			}
			if (IsKeyPressed(KEY_THREE)) {
				currentShape = AsteroidShape::PENTAGON;
			}
			if (IsKeyPressed(KEY_FOUR)) {
				currentShape = AsteroidShape::RANDOM;
			}
			if (IsKeyPressed(KEY_C)) {
				shootDir = static_cast<ShootDir>((static_cast<int>(shootDir) + 1) % 4);
			}

			// Weapon switch
			if (IsKeyPressed(KEY_TAB)) {
				int next = (static_cast<int>(currentWeapon) + 1) % static_cast<int>(WeaponType::COUNT);
				if (next == static_cast<int>(WeaponType::SPECIAL)) next = 0; // pomiń SPECIAL
				currentWeapon = static_cast<WeaponType>(next);
			}

			// Shooting
			{
				if (player->IsAlive() && IsKeyDown(KEY_SPACE)) {
					shotTimer += dt;
					float interval = 1.f / player->GetFireRate(currentWeapon);
					float projSpeed = player->GetSpacing(currentWeapon) * player->GetFireRate(currentWeapon);

					while (shotTimer >= interval) {
						Vector2 p = player->GetPosition();
						p.y -= player->GetRadius();

						// Kierunek strzału
						Vector2 dir = { 0, -1 }; // domyślnie góra
						switch (shootDir) {
						case ShootDir::UP:    dir = { 0, -1 }; break;
						case ShootDir::RIGHT: dir = { 1, 0 };  break;
						case ShootDir::DOWN:  dir = { 0, 1 };  break;
						case ShootDir::LEFT:  dir = { -1, 0 }; break;
						}
						// --- TU WKLEJ KOD ---
						int dmg = 10;
						float projSpeed = player->GetSpacing(currentWeapon) * player->GetFireRate(currentWeapon);
						switch (currentWeapon) {
						case WeaponType::LASER:
							dmg = 20;
							break;
						case WeaponType::BULLET:
							dmg = 10;
							break;
						case WeaponType::ROCKET:
							dmg = 40;
							projSpeed *= 0.6f;
							break;
						case WeaponType::PLASMA:
							dmg = 15;
							projSpeed *= 1.2f;
							break;
						default:
							break;
						}
						if (currentWeapon == WeaponType::PLASMA) {
							// Główny kierunek
							Vector2 velocity = Vector2Scale(dir, projSpeed);
							projectiles.push_back(Projectile(p, velocity, dmg, currentWeapon));
							// Dwa boczne pod kątem ±20 stopni
							float angle = atan2f(dir.y, dir.x);
							float offset = 20.0f * (PI / 180.0f); // 20 stopni w radianach
							for (float a : { -offset, offset }) {
								float newAngle = angle + a;
								Vector2 newDir = { cosf(newAngle), sinf(newAngle) };
								Vector2 newVel = Vector2Scale(newDir, projSpeed);
								projectiles.push_back(Projectile(p, newVel, dmg, currentWeapon));
							}
						}
						else {
							Vector2 velocity = Vector2Scale(dir, projSpeed);
							projectiles.push_back(Projectile(p, velocity, dmg, currentWeapon));
						}
						Vector2 velocity = Vector2Scale(dir, projSpeed);
						projectiles.push_back(Projectile(p, velocity, dmg, currentWeapon));
						// --- KONIEC ---

						shotTimer -= interval;
					}
				}
				if (player->IsAlive() && healthpacks > 0 && IsKeyPressed(KEY_H)) {
					usedHealthpack = true;
					// Odzyskaj 20 HP, ale nie przekraczaj 100
					int newHP = player->GetHP() + 20;
					if (newHP > 100) newHP = 100;
					// Ustaw nowe HP (potrzebna metoda SetHP)
					player->SetHP(newHP);
					healthpacks--;
				}
				// Wystrzał specjalnego pocisku
				if (player->IsAlive() && specialReady && IsKeyPressed(KEY_R)) {
					usedSpecial = true;
					Vector2 p = player->GetPosition();
					p.y -= player->GetRadius();
					Vector2 dir = { 0, -1 };
					switch (shootDir) {
					case ShootDir::UP:    dir = { 0, -1 }; break;
					case ShootDir::RIGHT: dir = { 1, 0 };  break;
					case ShootDir::DOWN:  dir = { 0, 1 };  break;
					case ShootDir::LEFT:  dir = { -1, 0 }; break;
					}
					float projSpeed = 600.0f;
					int dmg = 100;
					Vector2 velocity = Vector2Scale(dir, projSpeed);
					projectiles.push_back(Projectile(p, velocity, dmg, WeaponType::SPECIAL));
					specialReady = false;
					specialCharge = 0;
				}

				else {
					float maxInterval = 1.f / player->GetFireRate(currentWeapon);

					if (shotTimer > maxInterval) {
						shotTimer = fmodf(shotTimer, maxInterval);
					}
				}
			}

			// Spawn asteroids
			if (spawnTimer >= spawnInterval && asteroids.size() < MAX_AST) {
				asteroids.push_back(MakeAsteroid(C_WIDTH, C_HEIGHT, currentShape));
				spawnTimer = 0.f;
				spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
			}

			// Update projectiles - check if in boundries and move them forward
			{
				auto projectile_to_remove = std::remove_if(projectiles.begin(), projectiles.end(),
					[dt](auto& projectile) {
						return projectile.Update(dt);
					});
				projectiles.erase(projectile_to_remove, projectiles.end());
			}

			// Projectile-Asteroid collisions O(n^2)
			for (auto pit = projectiles.begin(); pit != projectiles.end();) {
				bool removed = false;

				for (auto ait = asteroids.begin(); ait != asteroids.end(); ++ait) {
					float dist = Vector2Distance((*pit).GetPosition(), (*ait)->GetPosition());
					if (dist < (*pit).GetRadius() + (*ait)->GetRadius()) {

						BigAsteroid* big = dynamic_cast<BigAsteroid*>(ait->get());
						if (big) {
							big->hp -= pit->GetDamage();
							if (big->hp > 0) {
								// Nie usuwaj asteroidy, usuń tylko pocisk (jeśli nie jest specjalny)
								if (pit->GetDamage() != 100 || pit->GetRadius() != 18.f) {
									pit = projectiles.erase(pit);
									removed = true;
								}
								break;
							}
						}

						// Usuwamy asteroidę tylko jeśli nie jest BigAsteroid lub jej hp <= 0
						if (!big || big->hp <= 0) {
							if (big && !usedHealthpack && !usedSpecial) {
								gameEnded = true;
							}
							ait = asteroids.erase(ait);

							// Usuwaj tylko jeśli to NIE jest pocisk specjalny
							if (pit->GetDamage() != 100 || pit->GetRadius() != 18.f) {
								pit = projectiles.erase(pit);
								removed = true;
							}

							// Liczniki
							destroyedObstacles++;
							if (destroyedObstacles >= 15) {
								healthpacks++;
								destroyedObstacles = 0;
							}
							if (!specialReady) {
								specialCharge++;
								if (specialCharge >= 10) {
									specialReady = true;
									specialCharge = 10;
								}
							}
							destroyedAsteroids++;
							if (destroyedAsteroids >= 30 && !bigAsteroidSpawned) {
								asteroids.push_back(std::make_unique<BigAsteroid>(C_WIDTH, C_HEIGHT));
								bigAsteroidSpawned = true;
							}
						}
						if (removed) break;
					}
				}
				if (!removed) {
					++pit;
				}
			}

			// Asteroid-Ship collisions
			{
				auto remove_collision =
					[&player, dt](auto& asteroid_ptr_like) -> bool {
					if (player->IsAlive()) {
						float dist = Vector2Distance(player->GetPosition(), asteroid_ptr_like->GetPosition());

						if (dist < player->GetRadius() + asteroid_ptr_like->GetRadius()) {
							player->TakeDamage(asteroid_ptr_like->GetDamage());
							return true; // Mark asteroid for removal due to collision
						}
					}
					if (!asteroid_ptr_like->Update(dt)) {
						return true;
					}
					return false; // Keep the asteroid
					};
				auto asteroid_to_remove = std::remove_if(asteroids.begin(), asteroids.end(), remove_collision);
				asteroids.erase(asteroid_to_remove, asteroids.end());
			}

			// Render everything
			{
				Renderer::Instance().Begin();
				const char* dirName = "";
				switch (shootDir) {
				case ShootDir::UP:    dirName = "UP"; break;
				case ShootDir::RIGHT: dirName = "RIGHT"; break;
				case ShootDir::DOWN:  dirName = "DOWN"; break;
				case ShootDir::LEFT:  dirName = "LEFT"; break;
				}
				DrawText(TextFormat("Shoot Dir: %s", dirName), 10, 70, 20, YELLOW);

				DrawText(TextFormat("HP: %d", player->GetHP()),
					10, 10, 20, GREEN);
				DrawText(TextFormat("Special: %d/10%s", specialCharge, specialReady ? " (READY!)" : ""), 10, 100, 20, specialReady ? ORANGE : GRAY);
				DrawText(TextFormat("Healthpacks: %d (H to use)", healthpacks), 10, 130, 20, LIGHTGRAY);
				DrawText(TextFormat("Destroyed Asteroids: %d", destroyedAsteroids), 10, 160, 20, RED);
				DrawText(TextFormat("BigAsteroid spawned: %s", bigAsteroidSpawned ? "YES" : "NO"), 10, 190, 20, ORANGE);

				const char* weaponName = "";
				switch (currentWeapon) {
				case WeaponType::LASER: weaponName = "LASER"; break;
				case WeaponType::BULLET: weaponName = "BULLET"; break;
				case WeaponType::ROCKET: weaponName = "ROCKET"; break;
				case WeaponType::PLASMA: weaponName = "PLASMA"; break;
				default: weaponName = "LASER"; break;
				}
				DrawText(TextFormat("Weapon: %s", weaponName), 10, 40, 20, BLUE);

				for (const auto& projPtr : projectiles) {
					projPtr.Draw();
				}
				for (const auto& astPtr : asteroids) {
					astPtr->Draw();
				}

				player->Draw();
				if (!player->IsAlive()) {
					const char* msg = "git gud";
					int fontSize = 60;
					int textWidth = MeasureText(msg, fontSize);
					int x = (C_WIDTH - textWidth) / 2;
					int y = (C_HEIGHT - fontSize) / 2;
					DrawText(msg, x, y, fontSize, RED);
				}
				Renderer::Instance().End();
			}
		}
		UnloadTexture(downloadTexture);
	}

private:
	bool usedHealthpack = false;
	bool usedSpecial = false;
	bool gameEnded = false;
	Texture2D downloadTexture{};
	int destroyedAsteroids = 0;
	bool bigAsteroidSpawned = false;
	int healthpacks = 0;
	int destroyedObstacles = 0;
	int specialCharge = 0;
	bool specialReady = false;
	Application()
	{
		asteroids.reserve(1000);
		projectiles.reserve(10'000);
	};

	std::vector<std::unique_ptr<Asteroid>> asteroids;
	std::vector<Projectile> projectiles;

	AsteroidShape currentShape = AsteroidShape::TRIANGLE;

	static constexpr int C_WIDTH = 1200;
	static constexpr int C_HEIGHT = 800;
	static constexpr size_t MAX_AST = 150;
	static constexpr float C_SPAWN_MIN = 0.5f;
	static constexpr float C_SPAWN_MAX = 3.0f;

	static constexpr int C_MAX_ASTEROIDS = 1000;
	static constexpr int C_MAX_PROJECTILES = 10'000;
};

int main() {
	Application::Instance().Run();
	return 0;
}
