#include "Player.h"
#include "World/Chunks/ChunkContainer.h"
#include "pch.h"
#include "utils/utils.h"
#include <SFML/Graphics/Shader.hpp>

Player::Player(const sf::Vector3f& position, const sf::RenderTarget& target, sf::Shader& shader,
               ChunkManager& chunkManager)
    : mCamera(target, shader)
    , mPosition({position.x, position.y, position.z})
    , mAABB({Block::BLOCK_SIZE * 0.5f, Block::BLOCK_SIZE * 1.8f, Block::BLOCK_SIZE * 0.5f})
    , mWaterInWaterEffect(sf::Vector2f(target.getSize().x, target.getSize().y))
    , mSelectedBlock()
    , mChunkManager(chunkManager)
    , mCrosshairTexture()
    , mCrosshair()
{
    mCrosshairTexture.loadFromFile("resources/textures/crosshair.png");
    mCrosshair.setTexture(mCrosshairTexture);
    centerOrigin(mCrosshair);
    mCrosshair.setPosition(target.getSize().x / 2.f, target.getSize().y / 2.f);

    mWireframeShader.loadFromFile("resources/shaders/WireframeRenderer/VertexShader.shader",
                                  "resources/shaders/WireframeRenderer/GeometryShader.shader",
                                  "resources/shaders/WireframeRenderer/FragmentShader.shader");

    auto waterColor = sf::Color(49, 103, 189, 120);
    mWaterInWaterEffect.setFillColor(waterColor);
}

void Player::update(const float& deltaTime)
{
    mCamera.update(deltaTime);
    mCamera.updateViewProjection(mWireframeShader);
    mSelectedBlock.update(deltaTime, mCamera, mChunkManager);
}

void Player::fixedUpdate(const float& deltaTime, const ChunkContainer& chunkContainer)
{
    updateVelocity(deltaTime);
    updatePhysics(chunkContainer);

    mCamera.cameraPosition({mPosition.x, mPosition.y + PLAYER_HEIGHT, mPosition.z});
}
void Player::updatePhysics(const ChunkContainer& chunkContainer)
{
    updatePositionCheckingPhysicalCollisions(chunkContainer);
    updateInformationIfPlayerIsInWater(chunkContainer);
}

void Player::updateInformationIfPlayerIsInWater(const ChunkContainer& chunkContainer)
{
    if (doesPlayerTouchesWater(chunkContainer))
    {
        mIsPlayerInWater = true;
        updateInformationIfPlayersEyesAreInWater(chunkContainer);
    }
    else
    {
        mIsPlayerInWater = false;
        mArePlayerEyesInWater = false;
    }
}
void Player::updateInformationIfPlayersEyesAreInWater(const ChunkContainer& chunkContainer)
{
    mArePlayerEyesInWater =
        doesItCollideWithGivenNonAirBlock(aabbHeadAboveEyes(), chunkContainer, BlockId::Water);
}

bool Player::doesItCollideWithGivenNonAirBlock(const AABB& aabb,
                                               const ChunkContainer& chunkContainer,
                                               BlockId blockId) const
{
    auto nonAirBlocksPlayerTouched = chunkContainer.nonAirBlocksItTouches(aabb);
    return std::find_if(nonAirBlocksPlayerTouched.cbegin(), nonAirBlocksPlayerTouched.cend(),
                        [&blockId](const auto& block)
                        {
                            return block.blockId() == blockId;
                        }) != nonAirBlocksPlayerTouched.cend();
}

AABB Player::aabbHeadAboveEyes() const
{
    auto collisionBoxSize = mAABB.collisionBoxSize();
    collisionBoxSize.y = PLAYER_EYES_LEVEL;

    auto position = mPosition;
    position.y += PLAYER_HEIGHT - PLAYER_EYES_LEVEL;

    auto aabb = AABB(collisionBoxSize);
    aabb.updatePosition(position, AABB::RelativeTo::BottomCenter);
    return aabb;
}

bool Player::doesPlayerTouchesWater(const ChunkContainer& chunkContainer) const
{
    return doesItCollideWithGivenNonAirBlock(collisionBox(), chunkContainer, BlockId::Water);
}

void Player::updatePositionCheckingPhysicalCollisions(const ChunkContainer& chunkContainer)
{
    tryUpdatePositionByApplyingVelocityIfCollisionAllows(chunkContainer, mPosition.x, mVelocity.x);

    mIsPlayerOnGround = tryUpdatePositionByApplyingVelocityIfCollisionAllows(
        chunkContainer, mPosition.y, mVelocity.y);

    tryUpdatePositionByApplyingVelocityIfCollisionAllows(chunkContainer, mPosition.z, mVelocity.z);
}

bool Player::tryUpdatePositionByApplyingVelocityIfCollisionAllows(
    const ChunkContainer& chunkContainer, float& position, float& velocity)
{
    position += velocity;
    mAABB.updatePosition(mPosition, AABB::RelativeTo::BottomCenter);
    if (chunkContainer.doesItCollide(collisionBox()))
    {
        position -= velocity;
        velocity = 0;
        return true;
    }
    mAABB.updatePosition(mPosition, AABB::RelativeTo::BottomCenter);
    return false;
}

void Player::handleMovementKeyboardInputs(const float& deltaTime)
{
    constexpr auto ACCELERATION_RATIO = 0.1f;
    auto playerSpeed = mIsPlayerInWater ? PLAYER_WALKING_SPEED_IN_WATER : PLAYER_WALKING_SPEED;
    const auto finalSpeed = playerSpeed * ACCELERATION_RATIO * deltaTime;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
    {
        mVelocity += finalSpeed * mCamera.directionWithoutPitch();
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
    {
        mVelocity += -(finalSpeed * mCamera.directionWithoutPitch());
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
    {
        mVelocity += finalSpeed * mCamera.rightDirectionWithoutPitch();
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
    {
        mVelocity += -(finalSpeed * mCamera.rightDirectionWithoutPitch());
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && mIsPlayerInWater)
    {
        mVelocity += (finalSpeed * 0.5f * mCamera.upwardDirection());
    }
}

void Player::handleEvent(const sf::Event& event)
{
    switch (event.type)
    {
        case sf::Event::KeyPressed: handleKeyboardEvents(event); break;
        case sf::Event::MouseButtonPressed: handleMouseEvents(event); break;
    }
}
void Player::handleKeyboardEvents(const sf::Event& event)
{
    if (event.key.code == sf::Keyboard::Space)
    {
        tryJump();
    }
}

void Player::handleMouseEvents(const sf::Event& event)
{
    switch (event.mouseButton.button)
    {
        case sf::Mouse::Left:
        {
            tryDestroyBlock();
            break;
        }
        case sf::Mouse::Right:
        {
            tryPlaceBlock();
            break;
        }
    }
}

void Player::tryJump()
{
    if (mIsPlayerOnGround && !mIsPlayerInWater)
    {
        mVelocity.y = PLAYER_JUMP_FORCE * 0.1f;
    }
}

void Player::tryDestroyBlock()
{
    if (mSelectedBlock.isAnyBlockHighlighted())
    {
        mChunkManager.chunks().removeWorldBlock(mSelectedBlock.blockPosition());
    }
}

void Player::tryPlaceBlock()
{
    if (mSelectedBlock.isAnyBlockHighlighted())
    {
        auto relativeDirectionWhereBlockToBePlaced =
            Block::directionOfFace(mSelectedBlock.blockFace());

        auto coordinatesOfBlockToBePlaced =
            mSelectedBlock.blockPosition().coordinateInGivenDirection(
                relativeDirectionWhereBlockToBePlaced);

        if (!doesPlayerCollideWithBlock(coordinatesOfBlockToBePlaced))
        {
            mChunkManager.chunks().tryToPlaceBlock(
                BlockId::Dirt, coordinatesOfBlockToBePlaced,
                {HighlightedBlock::BLOCKS_THAT_MIGHT_BE_OVERPLACED});
        }
    }
}

bool Player::doesPlayerCollideWithBlock(const Block::Coordinate& coordinates) const
{
    auto blockToBePlacedAABB =
        AABB(sf::Vector3f(Block::BLOCK_SIZE, Block::BLOCK_SIZE, Block::BLOCK_SIZE));

    blockToBePlacedAABB.updatePosition(coordinates.nonBlockMetric(),
                                       AABB::RelativeTo::LeftBottomBack);

    return collisionBox().intersect(blockToBePlacedAABB);
}

void Player::updateVelocity(const float& deltaTime)
{
    handleMovementKeyboardInputs(deltaTime);
    decelerateVelocity(deltaTime);
    manageVerticalVelocity(deltaTime);
    limitVelocity(deltaTime);
}

void Player::decelerateVelocity(const float& deltaTime)
{
    mVelocity.x = mVelocity.x - mVelocity.x * PLAYER_WALKING_DECELERATE_RATIO * deltaTime;
    mVelocity.z = mVelocity.z - mVelocity.z * PLAYER_WALKING_DECELERATE_RATIO * deltaTime;
}

void Player::manageVerticalVelocity(const float& deltaTime)
{
    if (mIsPlayerInWater)
    {
        mVelocity.y -= 0.004f * deltaTime;
    }
    else
    {
        mVelocity.y -= (abs(mVelocity.y) * 0.1f + 0.3f) * deltaTime;
    }
}

void Player::limitVelocity(const float& deltaTime)
{
    static auto limitVelocitySpeed = [](auto& velocity, const auto& maxVelocity)
    {
        if (velocity > maxVelocity)
        {
            velocity = maxVelocity;
        }
        else if (-velocity > maxVelocity)
        {
            velocity = -maxVelocity;
        }
    };

    auto playerVelocity = PLAYER_MAX_HORIZONTAL_SPEED * deltaTime;
    limitVelocitySpeed(mVelocity.x, playerVelocity);
    limitVelocitySpeed(mVelocity.z, playerVelocity);

    if (mIsPlayerInWater && mVelocity.y > playerVelocity)
    {
        mVelocity.y = playerVelocity;
    }

    if (-mVelocity.y > PLAYER_MAX_FALLING_SPEED)
    {
        mVelocity.y = -PLAYER_MAX_FALLING_SPEED;
    }
}

glm::vec3 Player::position() const
{
    return mPosition;
}

const AABB& Player::collisionBox() const
{
    return mAABB;
}

Camera& Player::camera()
{
    return mCamera;
}

void Player::draw(const Renderer3D& renderer3D, sf::RenderTarget& target,
                  sf::RenderStates states) const
{
    if (mIsPlayerInWater && mArePlayerEyesInWater)
    {
        SfmlDraw(mWaterInWaterEffect, target, states);
    }

    mSelectedBlock.draw(renderer3D, mWireframeShader);
    SfmlDraw(mCrosshair, target, states);
}
