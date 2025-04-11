// matrix.h
#ifndef MATRIX_H
#define MATRIX_H
void createTranslationMatrix(float tx, float ty, float tz, float* matrix);
void createOrthographicMatrix(float left, float right, float bottom, float top, float near,
                              float far, float* matrix);
#endif
