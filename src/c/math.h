#ifndef MATH_H
#define MATH_H

#include "types.h"

#define TURN_RANGE 90.0f
#define PI 3.14159f

Vec4 scalar_multiplication(float scale, Vec4 v){
	v.x = v.x*scale;
	v.y = v.y*scale;
	v.z = v.z*scale;
	
	return v;
}

Vec4 vec_add(Vec4 v, Vec4 u){
	Vec4 result;
	result.x = v.x + u.x;
	result.y = v.y + u.y;
	result.z = v.z + u.z;
	
	return result;
}

void vec_clear(Vec4 *v){
	v->x = 0;
	v->y = 0;
	v->z = 0;
	v->w = 0;
}

void vec_assign(Vec4 to, Vec4 from){
	to.x = from.x;
	to.y = from.y;
	to.z = from.z;
	to.w = from.w;
}


float power (float x, int y){
	float sum = x;
	if (y < 0){
		for (int i = -1; i > y; i--){
			sum = sum * x;
		}
		sum = 1.0f / sum;
	}
	else{
		for (int i = 1; i < y; i++){
			sum = sum * x;
		}
	}
	
	return sum;
}

int factorial(int x){
	int sum = 1;
	for (int i = 1; i <= x; i++){
		sum *= i;
	}
	return sum;
}

float sin(float v){
	while (v > PI) v -= 2.0f * PI;
	while (v < -PI) v += 2.0f * PI;

	float x,y,sum = 0.0f;
	int z = 0;
	for (int i = 1; i < 11; i += 2){
		y = power(v, i);
		z = factorial(i);
		x = y / (float)z;
		if (((i - 1) / 2) % 2 != 0){
			sum = sum - x;
		}
		else{
			sum = sum + x;
		}
	}	
	return sum;
}

float cos(float v){
	return sin(PI / 2.0f - v);
}

Vec4 cross(Vec4 a, Vec4 b) {
	Vec4 result;
	result.x = (a.y * b.z - a.z * b.y);
	result.y = (a.z * b.x - a.x * b.z); 
	result.z = (a.x * b.y - a.y * b.x);
	return result;
}

float dot(Vec4 a, Vec4 b){
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

float sqrt(float x) {
    if (x <= 0.0f) return 0.0f;
    float guess = x;
    for (int i = 0; i < 10; i++) {
        guess = (guess + x / guess) * 0.5f;
    }
    return guess;
}

float length(Vec4 v) {
	return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vec4 normalize(Vec4 v) {
	return scalar_multiplication((1.0f / length(v)), v);
}

void identity(Vec4 v) {
	v.x = 1;
	v.y = 1;
	v.z = 1;
	v.w = 0;
}


#endif