#ifndef PHYSICS2D_GJK_H
#define PHYSICS2D_GJK_H

#include "physics2d_common.h"
#include "physics2d_shape.h"
#include "physics2d_simplex.h"

#include "physics2d_capsule.h"
#include "physics2d_circle.h"
#include "physics2d_curve.h"
#include "physics2d_edge.h"
#include "physics2d_ellipse.h"
#include "physics2d_point.h"
#include "physics2d_polygon.h"
#include "physics2d_rectangle.h"
#include "physics2d_sector.h"

namespace Physics2D
{
	struct PenetrationInfo
	{
		Vector2 normal;
		real penetration = 0;
	};

	struct PenetrationSource
	{
		Vector2 a1;
		Vector2 a2;
		Vector2 b1;
		Vector2 b2;
	};

	struct PointPair
	{
		PointPair() = default;
		Vector2 pointA;
		Vector2 pointB;
		bool isEmpty()const
		{
			return pointA.fuzzyEqual({ 0, 0 }) && pointB.fuzzyEqual({ 0, 0 });
		}
		bool operator==(const PointPair& other)const
		{
			return (other.pointA.fuzzyEqual(this->pointA) && other.pointB.fuzzyEqual(this->pointB))
				|| (other.pointB.fuzzyEqual(this->pointA) && other.pointA.fuzzyEqual(this->pointB));
		}
	};

	namespace Narrowphase
	{
		Simplex gjk(const ShapePrimitive& shapeA, const ShapePrimitive& shapeB, const size_t& iteration = 20);
		SimplexVertex support(const ShapePrimitive& shapeA, const ShapePrimitive& shapeB, const Vector2& direction);
		Vector2 findFarthestPoint(const ShapePrimitive& shape, const Vector2& direction);
		Vector2 calculateDirectionByEdge(const SimplexVertex& v1, const SimplexVertex& v2, bool pointToOrigin);
		std::pair<Vector2, Index> findFarthestPoint(const Container::Vector<Vector2>& vertices, const Vector2& direction);
		void degradeSimplex(Simplex& simplex);
	}
	/// <summary>
	/// GJKHelper Collision Detection Class
	/// </summary>
	class GJKHelper
	{
	public:
		/// <summary>
		/// Gilbert-Johnson-Keerthi distance algorithm
		/// </summary>
		/// <param name="shape_A"></param>
		/// <param name="shape_B"></param>
		/// <param name="iteration"></param>
		/// <returns>return initial simplex and whether collision exists</returns>
		static std::tuple<bool, SimplexVertexArray> gjk(const ShapePrimitive& shapeA, const ShapePrimitive& shapeB,
		                                     const size_t& iteration = 20);
		/// <summary>
		/// Expanding Polygon Algorithm
		/// </summary>
		/// <param name="shape_A"></param>
		/// <param name="shape_B"></param>
		/// <param name="src">initial simplex</param>
		/// <param name="iteration">iteration times</param>
		/// <param name="epsilon">epsilon of iterated result</param>
		/// <returns>return expanded simplex</returns>
		static SimplexVertexArray epa(const ShapePrimitive& shapeA, const ShapePrimitive& shapshapeBe_B, const SimplexVertexArray& src,
		                   const size_t& iteration = 20, const real& epsilon = Constant::GeometryEpsilon);
		/// <summary>
		/// Dump collision penetration normal and depth
		/// </summary>
		/// <param name="shape_A"></param>
		/// <param name="shape_B"></param>
		/// <param name="simplex"></param>
		/// <returns></returns>
		static PenetrationInfo dumpInfo(const PenetrationSource& source);
		/// <summary>
		/// Support function.
		/// </summary>
		/// <param name="shape_A"></param>
		/// <param name="shape_B"></param>
		/// <param name="direction"></param>
		/// <returns></returns>
		static SimplexVertex support(const ShapePrimitive& shapeA, const ShapePrimitive& shapeB,
		                         const Vector2& direction);
		/// <summary>
		/// Find two points that can form an edge closest to origin of simplex
		/// </summary>
		/// <param name="simplex"></param>
		/// <returns></returns>
		static std::tuple<size_t, size_t> findEdgeClosestToOrigin(const SimplexVertexArray& simplex);
		/// <summary>
		/// Find farthest projection point in given direction
		/// </summary>
		/// <param name="shape"></param>
		/// <param name="direction"></param>
		/// <returns></returns>
		static Vector2 findFarthestPoint(const ShapePrimitive& shape, const Vector2& direction);
		/// <summary>
		/// Find farthest projection point in given direction
		/// </summary>
		/// <param name="vertices"></param>
		/// <param name="direction"></param>
		/// <returns></returns>
		static std::pair<Vector2, size_t> findFarthestPoint(const Container::Vector<Vector2>& vertices, const Vector2& direction);
		/// <summary>
		/// Adjust triangle simplex, remove the point that can not form a triangle that contains origin
		/// </summary>
		/// <param name="simplex"></param>
		/// <param name="closest_1"></param>
		/// <param name="closest_2"></param>
		/// <returns></returns>
		static std::optional<SimplexVertex> adjustSimplex(SimplexVertexArray& simplex, const size_t& closest_1,
		                                              const size_t& closest_2);
		/// <summary>
		/// Given two points, calculate the perpendicular vector and the orientation is user-defined.
		/// </summary>
		/// <param name="p1"></param>
		/// <param name="p2"></param>
		/// <param name="pointToOrigin"></param>
		/// <returns></returns>
		static Vector2 calculateDirectionByEdge(const Vector2& p1, const Vector2& p2, bool pointToOrigin = true);
		/// <summary>
		/// Calculate the distance of two shape primitive
		/// </summary>
		/// <param name="shapeA"></param>'
		/// <param name="shapeB"></param>
		/// <returns></returns>
		static PointPair distance(const ShapePrimitive& shapeA, const ShapePrimitive& shapeB,
		                          const real& iteration = 20, const real& epsilon = Constant::GeometryEpsilon);
		/// <summary>
		/// dump collision source point
		/// </summary>
		/// <param name="simplex"></param>
		/// <returns></returns>
		static PenetrationSource dumpSource(const SimplexVertexArray& simplex);
		/// <summary>
		/// dump contact points
		/// </summary>
		/// <param name="source"></param>
		/// <returns></returns>
		static PointPair dumpPoints(const PenetrationSource& source);
	};


}


#endif
