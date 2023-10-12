#include "PlayMode.hpp"

#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <random>
#include <array>

PlayMode::PlayMode(Client &client_) : client(client_) {
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.repeat) {
			//ignore repeats
		} else if (evt.key.keysym.sym == SDLK_a) {
			controls.left.downs += 1;
			controls.left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			controls.right.downs += 1;
			controls.right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			controls.up.downs += 1;
			controls.up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			controls.down.downs += 1;
			controls.down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			controls.fire.downs += 1;
			controls.fire.pressed = true;
			return true;
        } else if (evt.key.keysym.sym == SDLK_q) {
            controls.gravity_ccw.downs += 1;
            controls.gravity_ccw.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_e) {
            controls.gravity_cw.downs += 1;
            controls.gravity_cw.pressed = true;
            return true;
        }
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			controls.left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			controls.right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			controls.up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			controls.down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			controls.fire.pressed = false;
			return true;
        } else if (evt.key.keysym.sym == SDLK_q) {
            controls.gravity_ccw.pressed = false;
            return true;
        } else if (evt.key.keysym.sym == SDLK_e) {
            controls.gravity_cw.pressed = false;
            return true;
        }
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//queue data for sending to server:
	controls.send_controls_message(&client.connection);

	//reset button press counters:
	controls.left.downs = 0;
	controls.right.downs = 0;
	controls.up.downs = 0;
	controls.down.downs = 0;
	controls.fire.downs = 0;
    controls.gravity_ccw.downs = 0;
    controls.gravity_cw.downs = 0;

	//send/receive data:
	client.poll([this](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		} else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		} else { assert(event == Connection::OnRecv);
			//std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush(); //DEBUG
			bool handled_message;
			try {
				do {
					handled_message = false;
					if (game.recv_state_message(c)) handled_message = true;
				} while (handled_message);
			} catch (std::exception const &e) {
				std::cerr << "[" << c->socket << "] malformed message from server: " << e.what() << std::endl;
				//quit the game:
				throw e;
			}
		}
	}, 0.0);
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

	static std::array< glm::vec2, 4 > const triangle = {glm::vec2(1.0f, 0.0f), glm::vec2(-0.5f, 0.8660254038f), glm::vec2(0.0f, 0.0f), glm::vec2(-0.5f, -0.8660254038f)};

    static std::array< glm::vec2, 32 > const circle = [](){
        std::array< glm::vec2, 32 > ret;
        for (uint32_t a = 0; a < ret.size(); ++a) {
            float ang = a / float(ret.size()) * 2.0f * float(M_PI);
            ret[a] = glm::vec2(std::cos(ang), std::sin(ang));
        }
        return ret;
    }();

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	
	//figure out view transform to center the arena:
	float aspect = float(drawable_size.x) / float(drawable_size.y);
	float scale = std::min(
		2.0f * aspect / (Game::BulletArenaMax.x - Game::BulletArenaMin.x + 2.0f * Game::PlayerRadius),
		2.0f / (Game::BulletArenaMax.y - Game::BulletArenaMin.y + 2.0f * Game::PlayerRadius)
	);
	glm::vec2 offset = -0.5f * (Game::ArenaMax + Game::ArenaMin);

	glm::mat4 world_to_clip = glm::mat4(
		scale / aspect, 0.0f, 0.0f, offset.x,
		0.0f, scale, 0.0f, offset.y,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	{
		DrawLines lines(world_to_clip);

		//helper:
		auto draw_text = [&](glm::vec2 const &at, std::string const &text, float H) {
			lines.draw_text(text,
				glm::vec3(at.x, at.y, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			float ofs = (1.0f / scale) / drawable_size.y;
			lines.draw_text(text,
				glm::vec3(at.x + ofs, at.y + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		};

		lines.draw(glm::vec3(Game::ArenaMin.x, Game::ArenaMin.y, 0.0f), glm::vec3(Game::ArenaMax.x, Game::ArenaMin.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
		lines.draw(glm::vec3(Game::ArenaMin.x, Game::ArenaMax.y, 0.0f), glm::vec3(Game::ArenaMax.x, Game::ArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
		lines.draw(glm::vec3(Game::ArenaMin.x, Game::ArenaMin.y, 0.0f), glm::vec3(Game::ArenaMin.x, Game::ArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
		lines.draw(glm::vec3(Game::ArenaMax.x, Game::ArenaMin.y, 0.0f), glm::vec3(Game::ArenaMax.x, Game::ArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));

        lines.draw(glm::vec3(Game::BulletArenaMin.x, Game::BulletArenaMin.y, 0.0f), glm::vec3(Game::BulletArenaMax.x, Game::BulletArenaMin.y, 0.0f), glm::u8vec4(0xff, 0x00, 0x00, 0xff));
        lines.draw(glm::vec3(Game::BulletArenaMin.x, Game::BulletArenaMax.y, 0.0f), glm::vec3(Game::BulletArenaMax.x, Game::BulletArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0x00, 0xff));
        lines.draw(glm::vec3(Game::BulletArenaMin.x, Game::BulletArenaMin.y, 0.0f), glm::vec3(Game::BulletArenaMin.x, Game::BulletArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0x00, 0xff));
        lines.draw(glm::vec3(Game::BulletArenaMax.x, Game::BulletArenaMin.y, 0.0f), glm::vec3(Game::BulletArenaMax.x, Game::BulletArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0x00, 0xff));

		for (auto const &player : game.players) {
			glm::u8vec4 col = glm::u8vec4(player.color.x*255, player.color.y*255, player.color.z*255, 0xff);
            glm::u8vec4 col2 = glm::u8vec4(player.color.x*160, player.color.y*160, player.color.z*160, 0xff);

            if (&player == &game.players.front()) {
				//mark current player (which server sends first):
                for (uint32_t a = 0; a < circle.size(); ++a) {
                    lines.draw(
                            glm::vec3(player.position + Game::PlayerRadius * 1.2f * circle[a], 0.0f),
                            glm::vec3(player.position + Game::PlayerRadius * 1.2f * circle[(a+1)%circle.size()], 0.0f),
                            col
                    );
                }

//				lines.draw(
//					glm::vec3(player.position + Game::PlayerRadius * glm::vec2(-0.5f,-0.5f), 0.0f),
//					glm::vec3(player.position + Game::PlayerRadius * glm::vec2( 0.5f, 0.5f), 0.0f),
//					col
//				);
//				lines.draw(
//					glm::vec3(player.position + Game::PlayerRadius * glm::vec2(-0.5f, 0.5f), 0.0f),
//					glm::vec3(player.position + Game::PlayerRadius * glm::vec2( 0.5f,-0.5f), 0.0f),
//					col
//				);
			}
			for (uint32_t a = 0; a < circle.size(); ++a) {
				lines.draw(
					glm::vec3(player.position + Game::PlayerRadius * circle[a], 0.0f),
					glm::vec3(player.position + Game::PlayerRadius * circle[(a+1)%circle.size()], 0.0f),
					col
				);

                lines.draw(
                    glm::vec3(player.position + Game::PlayerGravityRadius * circle[a], 0.0f),
                    glm::vec3(player.position + Game::PlayerGravityRadius * circle[(a+1)%circle.size()], 0.0f),
                    col2
                );
			}
            for (uint32_t a = 0; a < triangle.size(); ++a) {
                float tx = triangle[a].x;
                float ty = triangle[a].y;
                float tx2 = triangle[(a+1)%triangle.size()].x;
                float ty2 = triangle[(a+1)%triangle.size()].y;
                lines.draw(
                        glm::vec3(player.position + Game::PlayerRadius * (glm::vec2(cos(player.angle) * tx + sin(player.angle) * ty, sin(player.angle) * tx - cos(player.angle) * ty)), 0.0f),
                        glm::vec3(player.position + Game::PlayerRadius * (glm::vec2(cos(player.angle) * tx2 + sin(player.angle) * ty2, sin(player.angle) * tx2 - cos(player.angle) * ty2)), 0.0f),
                        col
                );

                lines.draw(
                        glm::vec3(player.position + Game::PlayerGravityRadius * (glm::vec2(cos(player.grav_angle) * tx + sin(player.grav_angle) * ty, sin(player.grav_angle) * tx - cos(player.grav_angle) * ty)), 0.0f),
                        glm::vec3(player.position + Game::PlayerGravityRadius * (glm::vec2(cos(player.grav_angle) * tx2 + sin(player.grav_angle) * ty2, sin(player.grav_angle) * tx2 - cos(player.grav_angle) * ty2)), 0.0f),
                        col2
                );
            }

			draw_text(player.position + glm::vec2(-0.05f, -0.05f), std::to_string(player.score), 0.09f);
		}

        for (auto const &bullet: game.bullets) {
            glm::u8vec4 col = glm::u8vec4(bullet.color.x*255, bullet.color.y*255, bullet.color.z*255, 0xff);
            glm::u8vec4 col2 = glm::u8vec4(bullet.color.x*200, bullet.color.y*200, bullet.color.z*200, 0xff);
            for (uint32_t a = 0; a < circle.size(); ++a) {
                lines.draw(
                        glm::vec3(bullet.position + Game::BulletRadius * circle[a], 0.0f),
                        glm::vec3(bullet.position + Game::BulletRadius * circle[(a+1)%circle.size()], 0.0f),
                        col
                );

                lines.draw(
                        glm::vec3(bullet.position + Game::BulletRadius * 0.6f * circle[a], 0.0f),
                        glm::vec3(bullet.position + Game::BulletRadius * 0.6f * circle[(a+1)%circle.size()], 0.0f),
                        col2
                );
            }
        }
	}
	GL_ERRORS();
}
