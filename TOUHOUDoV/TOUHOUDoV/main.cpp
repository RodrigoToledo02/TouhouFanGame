////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Assignment:		Game Project - Touhou: Darkness of the Void
//  Instructor:     David Burchill
//  Year / Term:    Winter 2025
//  File name:      main.cpp
//
//  Student name:   Rodrigo Toledo
//  Student email:  rtoledocastillo01@mynbcc.ca / rodri_toledo32@outlook.com
//
//     I certify that this work is my work only, any work copied from Stack Overflow, textbooks,
//     or elsewhere is properly cited.
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////

//  Bug List
//  1.

#include <SFML/Graphics.hpp>
#include <iostream>
#include "GameEngine.h"

int main()
{
	GameEngine game("../config.txt");
	game.run();
	return 0;
}