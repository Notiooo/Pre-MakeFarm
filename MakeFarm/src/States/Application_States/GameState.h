#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <SFML/Graphics/Shader.hpp>

#include "Renderer3D/Renderer3D.h"
#include "Resources/TexturePack.h"
#include "States/State.h"
#include "Utils/Settings.h"
#include "World/Camera.h"
#include "World/Block/FacedBlock.h"
#include "World/Chunks/Chunk.h"
#include "World/Chunks/ChunkContainer.h"

class StateStack;

/**
 * \brief The game state in which the game world is created,
 * all objects are placed and the processes inside the game world are controlled.
 */
class GameState : public State
{
public:
	GameState(StateStack& stack, sf::RenderWindow& window);

	/**
	 * \brief Draws only this state to the passed target
	 * \param target where it should be drawn to
	 * \param states provides information about rendering process (transform, shader, blend mode)
	 */
	void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

	/**
	 * \brief Updates the status/logic of the state
	 * \param deltaTime the time that has passed since the last frame.
	 */
	bool fixedUpdate(const float& deltaTime) override;

    /**
     * \brief Updates the status/logic of the ImGui Debug Menu
     */
	void updateDebugMenu();

    /**
     * \brief Updates the status/logic of the state which, as a rule, should not depend on the time,
     * but should execute every single frame.
     */
	bool update() override;

	/**
	 * \brief It takes input (event) from the user and interprets it
	 * \param event user input
	 */
	bool handleEvent(const sf::Event& event) override;

private:
    /** Rendering */
	sf::RenderWindow& mGameWindow; //!< Window to which this status is displayed
	sf::Shader mShader;
	Camera mGameCamera;
	Renderer3D mGameRenderer;

    /** Settings */
	TexturePack mTexturePack;
	Settings mGameSettings;

    /** Utils */
    ChunkContainer mTestChunk;
	FacedBlock mSelectedBlock; //!< Marking the block the player is looking at
};


#endif