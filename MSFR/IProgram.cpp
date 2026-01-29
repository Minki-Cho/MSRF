#include "IProgram.h"
#include "Game/GameProgram.h"

util::owner<IProgram*> create_program(int viewport_width, int viewport_height)
{
    return new GameProgram(viewport_width, viewport_height);
}
