#include <vector>
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
	enum Size { SMALL = 1, MEDIUM = 2, LARGE = 4, GIGA = 10 } size = SMALL;
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
		ToggleFullscreen();
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
		float thickness = 4.0f;
		DrawPolyLinesEx(pos, sides, radius, rot, thickness, WHITE);
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

	float constexpr GetRadius() const {
		return 16.f * (float)render.size;
	}

	int GetDamage() const {
		return baseDamage * static_cast<int>(render.size);
	}

	int GetSize() const {
		return static_cast<int>(render.size);
	}

	int GetMaxHP() const {
		switch (render.size) {
			case Renderable::SMALL: return 25;
			case Renderable::MEDIUM: return 100;
			case Renderable::LARGE: return 300;
			case Renderable::GIGA: return 1000;
			default: return 1;
		}
	}

	void TakeDamage(int dmg) {
		hp -= dmg;
	}

	bool IsDead() const {
		return hp <= 0;
	}

protected:
	void init(int screenW, int screenH) {
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
	int hp;
	static constexpr float LIFE = 10.f;
	static constexpr float SPEED_MIN = 125.f;
	static constexpr float SPEED_MAX = 250.f;
	static constexpr float ROT_MIN = 50.f;
	static constexpr float ROT_MAX = 240.f;
};

class TriangleAsteroid : public Asteroid {
public:
	TriangleAsteroid(int w, int h) : Asteroid(w, h) {
		baseDamage = 5;
		render.size = Renderable::SMALL;
		hp = GetMaxHP();
	}
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 3, GetRadius(), transform.rotation);
	}
};
class SquareAsteroid : public Asteroid {
public:
	SquareAsteroid(int w, int h) : Asteroid(w, h) {
		baseDamage = 10;
		render.size = Renderable::MEDIUM;
		hp = GetMaxHP();
	}
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 4, GetRadius(), transform.rotation);
	}
};
class PentagonAsteroid : public Asteroid {
public:
	PentagonAsteroid(int w, int h) : Asteroid(w, h) {
		baseDamage = 15;
		render.size = Renderable::LARGE;
		hp = GetMaxHP();
	}
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 5, GetRadius(), transform.rotation);
	}
};
class GigaAsteroid : public Asteroid {
public:
	GigaAsteroid(int w, int h) : Asteroid(w, h) {
		baseDamage = 10;
		render.size = Renderable::GIGA;
		hp = GetMaxHP();
	}
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 9, GetRadius(), transform.rotation);
	}
};

// Shape selector
enum class AsteroidShape { TRIANGLE = 3, SQUARE = 4, PENTAGON = 5, GIGA = 6, RANDOM = 0 };

// Factory
static inline std::unique_ptr<Asteroid> MakeAsteroid(int w, int h, AsteroidShape shape) {
	switch (shape) {
	case AsteroidShape::TRIANGLE:
		return std::make_unique<TriangleAsteroid>(w, h);
	case AsteroidShape::SQUARE:
		return std::make_unique<SquareAsteroid>(w, h);
	case AsteroidShape::PENTAGON:
		return std::make_unique<PentagonAsteroid>(w, h);
	case AsteroidShape::GIGA:
		return std::make_unique<GigaAsteroid>(w, h);
	default: {
		int randomShape = GetRandomValue(0, 99);
		// Odds: 45% triangle, 30% square, 20% pentagon, 5% giga
		if (randomShape < 45) {
			return MakeAsteroid(w, h, AsteroidShape::TRIANGLE);
		}
		else if (randomShape < 75) {
			return MakeAsteroid(w, h, AsteroidShape::SQUARE);
		}
		else if (randomShape < 95) {
			return MakeAsteroid(w, h, AsteroidShape::PENTAGON);
		}
		else {
			return MakeAsteroid(w, h, AsteroidShape::GIGA);
		}
	}
	}
}

// --- PROJECTILE HIERARCHY ---
enum class WeaponType { LASER, BULLET, MINE, COUNT };
class Projectile {
public:
	Projectile(Vector2 pos, Vector2 vel, int dmg, WeaponType wt, float rotation, float time)
	{
		transform.position = pos;
		transform.rotation = rotation;
		physics.velocity = vel;
		baseDamage = dmg;
		type = wt;
		life = time;
	}
	bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));
		life -= dt;

		if (transform.position.x < 0 ||
			transform.position.x > Renderer::Instance().Width() ||
			transform.position.y < 0 ||
			transform.position.y > Renderer::Instance().Height())
		{
			return true;
		}
		if (life <= 0.f) {
			return true;
		}
		return false;
	}
	void Draw() const {
		if (type == WeaponType::BULLET) {
			DrawCircleV(transform.position, 5.f, WHITE);
		}
		else if (type == WeaponType::LASER) {
			static constexpr float LASER_LENGTH = 30.f;
			Rectangle lr = { transform.position.x, transform.position.y, 4.f, LASER_LENGTH };
			Vector2 origin = { 2.f, LASER_LENGTH / 2.f };
			DrawRectanglePro(lr, origin, 90.0f + transform.rotation * 180.0f / PI, RED);
		}
		else {
			DrawCircleV(transform.position, 10.f, GRAY); //mina
		}
	}
	Vector2 GetPosition() const {
		return transform.position;
	}

	float GetRadius() const {
		switch (type)
		{
		case WeaponType::LASER:
			return 2.f; // laser width
			break;
		case WeaponType::BULLET:
			return 5.f; // bullet radius
			break;
		case WeaponType::MINE:
			return 10.f; // mine radius
			break;
		default:
			break;
		}
	}

	int GetDamage() const {
		return baseDamage;
	}

private:
	TransformA transform;
	Physics    physics;
	int        baseDamage;
	WeaponType type;
	float      life;
};

inline static Projectile MakeProjectile(WeaponType wt,
	const Vector2 pos,
	float speed,
	float rotation)
{
	Vector2 vel{ cos(rotation)*speed, sin(rotation)*speed};
	if (wt == WeaponType::LASER) {
		return Projectile(pos, vel, 17, wt, rotation, 10.0f);
	}
	else if (wt == WeaponType::BULLET) {
		return Projectile(pos, vel, 25, wt, rotation, 10.0f);
	}
	else {
		return Projectile(pos, {0.0f, 0.0f}, 150, wt, rotation, 10.0f); // mina
	}
}

// --- SHIP HIERARCHY ---
class Ship {
public:
	Ship(int screenW, int screenH) {
		transform.position = {
			screenW * 0.5f,
			screenH * 0.5f
		};
		hp = 100;
		speed = 250.f;
		rSpeed = 70.f;
		alive = true;

		// per-weapon fire rate & spacing
		fireRateLaser = 18.f; // shots/sec
		fireRateBullet = 12.f;
		fireRateMine = 2.0f; // shots/sec
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
		switch (wt)
		{
		case WeaponType::LASER:
			return fireRateLaser;
			break;
		case WeaponType::BULLET:
			return fireRateBullet;
			break;
		case WeaponType::MINE:
			return fireRateMine;
			break;
		default:
			break;
		}
	}

	float GetSpacing(WeaponType wt) const {
		return (wt == WeaponType::LASER) ? spacingLaser : spacingBullet;
	}
	float GetAngle() const {
		return transform.rotation;
	}

protected:
	TransformA transform;
	int        hp;
	float      speed;
	float      rSpeed;
	bool       alive;
	float      fireRateLaser;
	float      fireRateBullet;
	float      fireRateMine;
	float      spacingLaser;
	float      spacingBullet;
};

class PlayerShip :public Ship {
public:
	PlayerShip(int w, int h) : Ship(w, h) {
		texture = LoadTexture("tekstury/spaceship1.png");
		GenTextureMipmaps(&texture);// Generate GPU mipmaps for a texture
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
			if (IsKeyDown(KEY_Q)) transform.rotation -= rSpeed * dt;
			if (IsKeyDown(KEY_E)) transform.rotation += rSpeed * dt;
		}
		else {
			transform.position.y += speed * dt;
		}
	}

	void Draw() const override {
		if (!alive && fmodf(GetTime(), 0.4f) > 0.2f) return;

		Rectangle srcRect = { 0, 0, (float)texture.width, (float)texture.height };
		// Destination rectangle: center at ship position, size scaled
		Rectangle dstRect = {
			transform.position.x,
			transform.position.y,
			texture.width * scale,
			texture.height * scale
		};
		// Origin: center of the destination rectangle
		Vector2 origin = { dstRect.width * 0.5f, dstRect.height * 0.5f };

		DrawTexturePro(texture, srcRect, dstRect, origin, transform.rotation, WHITE);
	}

	float GetRadius() const override {
		return (texture.height * scale) * 0.5f;
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

		InitWindow(100, 100, "DUMMY WINDOW");

		// Step 2: Get monitor size
		C_WIDTH = GetMonitorWidth(0);
		C_HEIGHT = GetMonitorHeight(0);

		// Step 3: Close dummy window and open real one
		CloseWindow();
		Renderer::Instance().Init(C_WIDTH, C_HEIGHT, "Asteroids OOP");

		Texture2D bg = LoadTexture("tekstury/space.jpg");

		auto player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);

		float spawnTimer = 0.f;
		float spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
		WeaponType currentWeapon = WeaponType::LASER;
		float shotTimer = 0.f;

		while (!WindowShouldClose()) {
			float dt = GetFrameTime();
			spawnTimer += dt;

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
			if (IsKeyPressed(KEY_FIVE)) {
				currentShape = AsteroidShape::RANDOM;
			}
			if (IsKeyPressed(KEY_FOUR)) {
				currentShape = AsteroidShape::GIGA;
			}

			// Weapon switch
			if (IsKeyPressed(KEY_TAB)) {
				currentWeapon = static_cast<WeaponType>((static_cast<int>(currentWeapon) + 1) % static_cast<int>(WeaponType::COUNT));
			}

			// Shooting
			{
				if (player->IsAlive() && IsKeyDown(KEY_SPACE)) {
					shotTimer += dt;
					float interval = 1.f / player->GetFireRate(currentWeapon);
					float projSpeed = player->GetSpacing(currentWeapon) * player->GetFireRate(currentWeapon);

					while (shotTimer >= interval) {
                        Vector2 p = player->GetPosition();
                        float rot_deg = player->GetAngle() - 90.0f;
                        float rot_rad = rot_deg * (PI / 180.0f);
                        p.x += cosf(rot_rad) * player->GetRadius();
                        p.y += sinf(rot_rad) * player->GetRadius();
                        projectiles.push_back(MakeProjectile(currentWeapon, p, projSpeed, rot_rad));
						shotTimer -= interval;
					}
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
						(*ait)->TakeDamage((*pit).GetDamage());
						if ((*ait)->IsDead()) {
							ait = asteroids.erase(ait);
						}
						pit = projectiles.erase(pit);
						removed = true;
						break;
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
				DrawTexturePro(bg, Rectangle{0, 0, (float)bg.width, (float)bg.height}, Rectangle{0, 0, (float)Renderer::Instance().Width(), (float)Renderer::Instance().Height()}, Vector2{0, 0}, 0.0f, WHITE);
				DrawText(TextFormat("HP: %d", player->GetHP()),
					10, 10, 20, GREEN);
				const char* weaponName = nullptr;
				switch (currentWeapon) {
				case WeaponType::LASER:
					weaponName = "LASER";
					break;
				case WeaponType::BULLET:
					weaponName = "BULLET";
					break;
				case WeaponType::MINE:
					weaponName = "MINE";
					break;
				default:
					weaponName = "UNKNOWN";
					break;
				}
				DrawText(TextFormat("Weapon: %s", weaponName),
					10, 40, 20, GREEN);
				const char* modeName = nullptr;
				switch (currentShape) {
				case AsteroidShape::TRIANGLE:
					modeName = "TRIANGLE";
					break;
				case AsteroidShape::SQUARE:
					modeName = "SQUARE";
					break;
				case AsteroidShape::PENTAGON:
					modeName = "PENTAGON";
					break;
				case AsteroidShape::GIGA:
					modeName = "GIGA";
					break;
				case AsteroidShape::RANDOM: default:
					modeName = "RANDOM";
					break;
				}
				DrawText(TextFormat("Mode: %s", modeName),
					10, 60, 20, GREEN);

				for (const auto& projPtr : projectiles) {
					projPtr.Draw();
				}
				for (const auto& astPtr : asteroids) {
					astPtr->Draw();
				}

				player->Draw();

				Renderer::Instance().End();
			}
		}
		UnloadTexture(bg);
	}

private:
	Application()
	{
		asteroids.reserve(C_MAX_ASTEROIDS);
		projectiles.reserve(C_MAX_PROJECTILES);
	};

	std::vector<std::unique_ptr<Asteroid>> asteroids;
	std::vector<Projectile> projectiles;

	AsteroidShape currentShape = AsteroidShape::RANDOM;

	int C_WIDTH = 900;
	int C_HEIGHT = 900;
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
