#pragma once

#include <glm/glm.hpp>

#include <string>
#include <list>
#include <random>

struct Connection;

//Game state, separate from rendering.

//Currently set up for a "client sends controls" / "server sends whole state" situation.

enum class Message : uint8_t {
	C2S_Controls = 1, //Greg!
	S2C_State = 's',
	//...
};

//used to represent a control input:
struct Button {
	uint8_t downs = 0; //times the button has been pressed
	bool pressed = false; //is the button pressed now
};

//state of one player in the game:
struct Player {
	//player inputs (sent from client):
	struct Controls {
		Button left, right, up, down, fire, gravity_cw, gravity_ccw;

		void send_controls_message(Connection *connection) const;

		//returns 'false' if no message or not a controls message,
		//returns 'true' if read a controls message,
		//throws on malformed controls message
		bool recv_controls_message(Connection *connection);
	} controls;

	//player state (sent from server):
	glm::vec2 position = glm::vec2(0.0f, 0.0f);
	glm::vec2 velocity = glm::vec2(0.0f, 0.0f);
    float angle = 0.0f;
    float grav_angle = 0.0f;
    int bullets_left = 5;
    float cooldown = 0.0f;
    int score = 0;

	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
	std::string name = "";
};

//state of one bullet in the game:
struct Bullet {
    //player state (sent from server):
    glm::vec2 position = glm::vec2(0.0f, 0.0f);
    glm::vec2 velocity = glm::vec2(0.0f, 0.0f);
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
    int bounces = 1;
    Player* player;
};

struct Game {
	std::list< Player > players; //(using list so they can have stable addresses)
    std::list< Bullet > bullets; //(using list so they can have stable addresses)
    Player *spawn_player(); //add player the end of the players list (may also, e.g., play some spawn anim)
	void remove_player(Player *); //remove player from game (may also, e.g., play some despawn anim)

    Bullet *spawn_bullet(Player&);
    void remove_bullet(Bullet *);

	std::mt19937 mt; //used for spawning players
	uint32_t next_player_number = 1; //used for naming players

	Game();

	//state update function:
	void update(float elapsed);

	//constants:
	//the update rate on the server:
	inline static constexpr float Tick = 1.0f / 60.0f;

	//arena size:
	inline static constexpr glm::vec2 ArenaMin = glm::vec2(-1.2f, -0.7f);
	inline static constexpr glm::vec2 ArenaMax = glm::vec2( 1.2f,  0.7f);
    inline static constexpr glm::vec2 BulletArenaMin = glm::vec2(-1.4f, -0.9f);
    inline static constexpr glm::vec2 BulletArenaMax = glm::vec2( 1.4f,  0.9f);

	//player constants:
    inline static constexpr float PlayerGravityRadius = 0.4f;
	inline static constexpr float PlayerRadius = 0.08f;
	inline static constexpr float PlayerSpeed = 0.25f;
	inline static constexpr float PlayerAccelHalflife = 0.75f;

    inline static constexpr float BulletRadius = 0.02f;


    //---- communication helpers ----

	//used by client:
	//set game state from data in connection buffer
	// (return true if data was read)
	bool recv_state_message(Connection *connection);

	//used by server:
	//send game state.
	//  Will move "connection_player" to the front of the front of the sent list.
	void send_state_message(Connection *connection, Player *connection_player = nullptr) const;
};
