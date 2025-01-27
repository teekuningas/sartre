#include <algorithm>

#include "matrix.h"

void createOrthographicMatrix(float left, float right, float bottom, float top, float near, float far, float* matrix)
{
	std::fill(matrix, matrix + 16, 0.0f);
	matrix[0] = 2.0f / (right - left);
	matrix[5] = 2.0f / (top - bottom);
	matrix[10] = -2.0f / (far - near);
	matrix[12] = -(right + left) / (right - left);
	matrix[13] = -(top + bottom) / (top - bottom);
	matrix[14] = -(far + near) / (far - near);
	matrix[15] = 1.0f;
}

void createTranslationMatrix(float tx, float ty, float tz, float* matrix)
{
	std::fill(matrix, matrix + 16, 0.0f);
	matrix[0] = 1.0f;
	matrix[5] = 1.0f;
	matrix[10] = 1.0f;
	matrix[12] = tx;
	matrix[13] = ty;
	matrix[14] = tz;
	matrix[15] = 1.0f;
}


