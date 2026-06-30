#pragma once

#include "base_types.h"

Vector2Int operator-(Vector2Int v);
Vector3Int operator-(Vector3Int v);
Vector4Int operator-(Vector4Int v);

Vector2Int operator+(Vector2Int v);
Vector3Int operator+(Vector3Int v);
Vector4Int operator+(Vector4Int v);

Vector2Int operator*(int s, Vector2Int v);
Vector3Int operator*(int s, Vector3Int v);
Vector4Int operator*(int s, Vector4Int v);

Vector2Int operator*(Vector2Int v, int s);
Vector3Int operator*(Vector3Int v, int s);
Vector4Int operator*(Vector4Int v, int s);

Vector2Int operator+(Vector2Int left, Vector2Int right);
Vector3Int operator+(Vector3Int left, Vector3Int right);
Vector4Int operator+(Vector4Int left, Vector4Int right);

Vector2Int operator-(Vector2Int left, Vector2Int right);
Vector3Int operator-(Vector3Int left, Vector3Int right);
Vector4Int operator-(Vector4Int left, Vector4Int right);

Vector2 operator-(Vector2 v);
Vector3 operator-(Vector3 v);
Vector4 operator-(Vector4 v);

Vector2 operator+(Vector2 v);
Vector3 operator+(Vector3 v);
Vector4 operator+(Vector4 v);

Vector2 operator*(f32 s, Vector2 v);
Vector3 operator*(f32 s, Vector3 v);
Vector4 operator*(f32 s, Vector4 v);

Vector2 operator*(Vector2 v, f32 s);
Vector3 operator*(Vector3 v, f32 s);
Vector4 operator*(Vector4 v, f32 s);

Vector2 operator+(Vector2 left, Vector2 right);
Vector3 operator+(Vector3 left, Vector3 right);
Vector4 operator+(Vector4 left, Vector4 right);

Vector2 operator-(Vector2 left, Vector2 right);
Vector3 operator-(Vector3 left, Vector3 right);
Vector4 operator-(Vector4 left, Vector4 right);
