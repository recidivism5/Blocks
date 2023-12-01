// noise1234
//
// Author: Stefan Gustavson, 2003-2005
// Modified by: Ian Bryant, 2023
// Contact: stefan.gustavson@liu.se
//
// This code was GPL licensed until February 2011.
// As the original author of this code, I hereby
// release it into the public domain.
// Please feel free to use it for whatever you want.
// Credit is appreciated where appropriate, and I also
// appreciate being told where this code finds any use,
// but you may do as you like.

/*
* This implementation is "Improved Noise" as presented by
* Ken Perlin at Siggraph 2002. The 3D function is a direct port
* of his Java reference code which was once publicly available
* on www.noisemachine.com (although I cleaned it up, made it
* faster and made the code more readable), but the 1D, 2D and
* 4D functions were implemented from scratch by me.
*
* This is a backport to C of my improved noise class in C++
* which was included in the Aqsis renderer project.
* It is highly reusable without source code modifications.
*
*/
#pragma once

#include <base.h>

//randomizes the perlin table
void seed_perlin_noise(int seed);

//---------------------------------------------------------------------
/** 1D float Perlin noise, SL "noise()"
*/
float perlin_noise_1d( float x );


//---------------------------------------------------------------------
/** 2D float Perlin noise.
*/
float perlin_noise_2d( float x, float y );


//---------------------------------------------------------------------
/** 3D float Perlin noise.
*/
float perlin_noise_3d( float x, float y, float z );

float fractal_perlin_noise_2d(float x, float y, int octaves);