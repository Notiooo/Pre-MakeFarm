#include "BlockMap.h"
#include "pch.h"

#include <filesystem>

#include "Utils/Settings.h"

const BlockMap& BlockMap::blockMap()
{
    static const BlockMap instance;

    return instance;
}

const BlockType& BlockMap::blockType(const BlockId& blockId) const
{
    return mBlockMap.at(blockId);
}

BlockMap::BlockMap()
{
    parseDirectory("resources/blocks");
}

void BlockMap::parseDirectory(const std::string& directoryName)
{
    Settings settingsFile;
    for (const auto& file: std::filesystem::recursive_directory_iterator(directoryName))
    {
        if (file.is_regular_file())
        {
            std::string filePath = file.path().string();
            if (filePath.size() > 4 && std::string(filePath.end() - 4, filePath.end()) == ".cfg")
            {
                BlockType blockType;
                std::string fileName = file.path().filename().string();
                fileName.erase(fileName.cend() - 4, fileName.cend());

                settingsFile.openFile(filePath);
                blockType.blockFileName = fileName;

                // Set name of the coordinateInGivenDirection
                if (settingsFile.isPresent("Name"))
                {
                    blockType.blockName = settingsFile.get<std::string>("Name");
                }
                else
                {
                    blockType.blockName = "Unnamed Block";
                }

                // Set main texture of the coordinateInGivenDirection
                if (settingsFile.isPresent("Texture"))
                {
                    auto generalTexture = settingsFile.get<Block::TextureId>("Texture");
                    for (auto i = 0; i < static_cast<int>(Block::Face::Counter); ++i)
                    {
                        blockType.textureId[static_cast<Block::Face>(i)] = generalTexture;
                    }
                }

                // Set sides
                if (settingsFile.isPresent("TextureSide"))
                {
                    auto sideTexture = settingsFile.get<Block::TextureId>("TextureSide");
                    blockType.textureId[Block::Face::Left] = sideTexture;
                    blockType.textureId[Block::Face::Right] = sideTexture;
                    blockType.textureId[Block::Face::Front] = sideTexture;
                    blockType.textureId[Block::Face::Back] = sideTexture;
                }

                auto setIfPresent = [&settingsFile, &blockType](const std::string& setting,
                                                                const Block::Face& blockFace)
                {
                    if (settingsFile.isPresent(setting))
                    {
                        blockType.textureId[blockFace] =
                            settingsFile.get<Block::TextureId>(setting);
                    }
                };

                // Set other texture faces of blocks (optional)
                setIfPresent("TextureLeft", Block::Face::Left);
                setIfPresent("TextureRight", Block::Face::Right);
                setIfPresent("TextureTop", Block::Face::Top);
                setIfPresent("TextureBottom", Block::Face::Bottom);
                setIfPresent("TextureFront", Block::Face::Front);
                setIfPresent("TextureBack", Block::Face::Back);

                // Set other properties
                if (settingsFile.isPresent("Transparent"))
                {
                    blockType.transparent = settingsFile.get<bool>("Transparent");
                }

                if (settingsFile.isPresent("Collidable"))
                {
                    blockType.collidable = settingsFile.get<bool>("Collidable");
                }

                if (settingsFile.isPresent("Id"))
                {
                    auto blockId = static_cast<BlockId>(settingsFile.get<int>("Id"));
                    blockType.blockId = blockId;
                    mBlockMap[blockId] = blockType;
                }

                settingsFile.closeFile();
            }
        }
    }
}
