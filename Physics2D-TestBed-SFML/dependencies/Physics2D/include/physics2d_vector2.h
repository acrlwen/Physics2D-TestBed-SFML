#ifndef MATH_LINEAR_VECTOR2_H
#define MATH_LINEAR_VECTOR2_H
#include "physics2d_common.h"
namespace Physics2D
{
	struct PHYSICS2D_API Vector2
	{
        Vector2(const real& _x = 0.0, const real& _y = 0.0);
        Vector2(const Vector2& copy);
        Vector2& operator=(const Vector2& copy);
		Vector2(Vector2&& other) = default;

        Vector2 operator+(const Vector2& rhs)const;
        Vector2 operator-(const Vector2& rhs)const;
        Vector2 operator-()const;
        Vector2 operator*(const int& factor)const;
        Vector2 operator*(const real& factor)const;
        Vector2 operator/(const real& factor)const;
        Vector2 operator/(const int& factor)const;

        Vector2& operator+=(const Vector2& rhs);
        Vector2& operator-=(const Vector2& rhs);
        Vector2& operator*=(const real& factor);
        Vector2& operator*=(const int& factor);
        Vector2& operator/=(const real& factor);
        Vector2& operator/=(const int& factor);

        bool operator==(const Vector2& rhs)const;
        bool operator!=(const Vector2& rhs)const;
        bool equal(const Vector2& rhs)const;
        bool fuzzyEqual(const Vector2& rhs, const real& epsilon = Constant::GeometryEpsilon)const;
        bool isOrigin(const real& epsilon = Constant::GeometryEpsilon)const;
        bool isSameQuadrant(const Vector2& rhs)const;
		
        real lengthSquare()const;
        real length()const;
        real theta()const;
		Vector2 normal()const;
        Vector2 negative()const;


        Vector2& set(const real& _x, const real& _y);
        Vector2& set(const Vector2& copy);
        Vector2& clear();
        Vector2& negate();
        Vector2& swap(Vector2& other) noexcept;

        Vector2& normalize();
        Vector2 perpendicular()const;

        real dot(const Vector2& rhs)const;
        real cross(const Vector2& rhs)const;

        static real dotProduct(const Vector2& lhs, const Vector2& rhs);
        static real crossProduct(const Vector2& lhs, const Vector2& rhs);
        static real crossProduct(const real& x1, const real& y1, const real& x2, const real& y2);
        static Vector2 crossProduct(const real& lhs, const Vector2& rhs);
        static Vector2 crossProduct(const Vector2& lhs, const real& rhs);
        static Vector2 lerp(const Vector2& lhs, const Vector2& rhs, const real& t);
        real x;
		real y;
	};
}
#endif
