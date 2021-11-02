#pragma once
#include <glm.hpp>

struct Tile
	{
	glm::vec2 P1;
	glm::vec2 P2;
	float Alpha;
	int LoD;
	glm::vec4 Color;

	Tile(glm::vec2 const &p1, glm::vec2 const &p2, glm::vec4 const &color)
		{
		P1 = p1;
		P2 = p2;
		Color = color;
		}
	};
