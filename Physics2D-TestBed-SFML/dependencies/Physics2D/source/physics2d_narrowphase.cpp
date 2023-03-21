#include "physics2d_narrowphase.h"
#include "physics2d_narrowphase.h"

namespace Physics2D
{
	Simplex Narrowphase::gjk(const ShapePrimitive& shapeA, const ShapePrimitive& shapeB, const size_t& iteration)
	{
		Simplex simplex;

		Vector2 direction = shapeB.transform.position - shapeA.transform.position;

		if (direction.fuzzyEqual({ 0, 0 }))
			direction.set(1, 1);
		//first
		SimplexVertex vertex = support(shapeA, shapeB, direction);
		simplex.addSimplexVertex(vertex);
		//second
		direction.negate();
		vertex = support(shapeA, shapeB, direction);
		simplex.addSimplexVertex(vertex);

		//check 1d simplex(line segment) contains origin
		//WARN: this can be used to check collision but not friendly with EPA, here adding perturbation to avoid 1d simplex

		if (simplex.containsOrigin())
		{
			//return simplex;

			direction.set(-direction.y + 1.0f, -direction.x - 1.5f);
			SimplexVertex v2 = support(shapeA, shapeB, direction);
			simplex.vertices[2] = v2;
		}
		//third
		size_t iter = 0;
		while (iter <= iteration)
		{
			//default closest edge is index 0 and index 1
			direction = findDirectionByEdge(simplex.vertices[0], simplex.vertices[1], true);
			vertex = support(shapeA, shapeB, direction);

			//find repeated vertex
			if (simplex.contains(vertex))
				break;

			//vertex does not pass origin
			if (vertex.result.dot(direction) <= 0)
				break;

			simplex.addSimplexVertex(vertex);

			//use barycentric coordinates to check contains origin and find closest edge
			const SimplexVertex va = simplex.vertices[0];
			const SimplexVertex vb = simplex.vertices[1];
			const SimplexVertex vc = simplex.vertices[2];
			const Vector2 a = va.result;
			const Vector2 b = vb.result;
			const Vector2 c = vc.result;
			const Vector2 ab = b - a;
			const Vector2 ac = c - a;
			const Vector2 bc = c - b;
			const real ab_length = ab.length();
			const real ac_length = ac.length();
			const real bc_length = bc.length();

			const real u_ac = -a.dot(ac.normal()) / ac_length;
			const real u_bc = -b.dot(bc.normal()) / bc_length;

			const real v_ac = 1 - u_ac;
			const real v_bc = 1 - u_bc;
			/*
			 * [Ax Bx Cx][u] = [0]
			 * [Ay By Cy][v] = [0]
			 * [ 1  1  1][w] = [1]
			 * solve for u,v,w
			 */
			const real det = a.y * b.x - a.x * b.y + a.x * c.y - a.y * c.x + b.y * c.x - c.y * b.x;
			assert(det != 0.0f);
			const real u = (b.y * c.x - c.y * b.x) / det;
			const real v = (c.y * a.x - a.y * c.x) / det;
			const real w = 1 - u - v;

			
			if (u_ac > 0 && v_ac > 0 && v <= 0)
			{
				simplex.vertices[1] = vc;
			}
			else if (u_bc > 0 && v_bc > 0 && u <= 0)
			{
				simplex.vertices[0] = vc;
			}
			else if (u > 0 && v > 0 && w > 0)
			{
				//in region ABC, origin is inside simplex
				simplex.isContainOrigin = true;

				//reorder simplex index, so that the closest edge is index 0 and index 1
				//h means height
				const real h_u = u / bc_length;
				const real h_v = v / ac_length;
				const real h_w = w / ab_length;
				const real min = Math::tripleMin(h_u, h_v, h_w);
				if (min == h_u)
				{
					//c b a
					simplex.vertices[0] = vc;
					simplex.vertices[2] = va;
				}
				else if (min == h_v)
				{
					//a c b
					simplex.vertices[1] = vc;
					simplex.vertices[2] = vb;
				}

				return simplex;
			}

			simplex.removeEnd();

			iter++;
		}
		return simplex;
	}

	CollisionInfo Narrowphase::epa(const Simplex& simplex, const ShapePrimitive& shapeA, const ShapePrimitive& shapeB, const size_t& iteration, const real& epsilon)
	{
		//return 1d simplex with edge closest to origin
		CollisionInfo info;
		info.simplex = simplex;
		info.simplex.removeEnd();
		

		auto iterNext = [](std::list<SimplexVertexWithOriginDistance>::iterator& targetIter,
			std::list<SimplexVertexWithOriginDistance>& list)
		{
			++targetIter;
			if (targetIter == list.end())
				targetIter = list.begin();
		};
		auto iterPrev = [](std::list<SimplexVertexWithOriginDistance>::iterator& targetIter,
			std::list<SimplexVertexWithOriginDistance>& list)
		{
			if (targetIter == list.begin())
				targetIter = list.end();
			--targetIter;
		};

		if(simplex.count == 2)
		{
			//add some perturbation to try to reconstruct simplex;
		}

		//initiate polytope
		std::list<SimplexVertexWithOriginDistance> &polytope = info.polytope;
		for(auto iter = simplex.vertices.begin(); iter != simplex.vertices.end(); ++iter)
		{
			auto next = iter + 1;
			if (next == simplex.vertices.end())
				next = simplex.vertices.begin();
			SimplexVertexWithOriginDistance pair;
			pair.vertex = *iter;
			pair.distance = GeometryAlgorithm2D::pointToLineSegment(iter->result, next->result, { 0, 0 })
				.length();
			polytope.emplace_back(pair);
		}

		//TODO: process simplex with 1d case(circle vs circle, circle vs ellipse)


		size_t iter = 0;
		Vector2 direction;
		SimplexVertex vertex;

		auto iterStart = polytope.begin();
		auto iterEnd = polytope.end();
		auto iterTemp = polytope.begin();


		while(++iter < iteration)
		{
			//closest edge index is set to index 0 and index 1
			direction = findDirectionByEdge(info.simplex.vertices[0], info.simplex.vertices[1], false);

			vertex = support(shapeA, shapeB, direction);

			//cannot find any new vertex
			if (info.simplex.contains(vertex))
			{
				Vector2 temp = -GeometryAlgorithm2D::pointToLineSegment(info.simplex.vertices[0].result, info.simplex.vertices[1].result
					, { 0, 0 });
				info.penetration = temp.length();
				info.normal = temp.normal();
				return info;
			}

			//set to begin
			iterTemp = iterStart;

			//calculate distance
			//iterTemp->vertex = result.vertices[0];
			bool useVertex = false;
			real dist1 = GeometryAlgorithm2D::pointToLineSegment(info.simplex.vertices[0].result, vertex.result, { 0,  0 })
				.length();
			if(realEqual(dist1, 0))
			{
				//almost same vertex, just replace old
				iterStart->vertex = vertex;
				useVertex = true;
			}
			else
				iterStart->distance = dist1;

			
			iterStart->distance = dist1;

			iterNext(iterTemp, polytope);
			//insert
			SimplexVertexWithOriginDistance pair;
			pair.vertex = vertex;

			real dist2 = GeometryAlgorithm2D::pointToLineSegment(vertex.result, iterTemp->vertex.result, { 0, 0 })
				.length();

			pair.distance = dist2;

			if(realEqual(dist2, 0) && !useVertex)
			{
				//almost same vertex, check if used, then just replace old
				iterTemp->vertex = vertex;
			}
			else if(!useVertex)
				polytope.insert(iterTemp, pair);

			//reset iterTemp for next loop
			iterTemp = iterStart;
			//find shortest distance and set iterStart
			real minDistance = Constant::Max;
			auto iterTarget = iterStart;
			while(true)
			{
				if (iterTemp->distance < minDistance)
				{
					minDistance = iterTemp->distance;
					iterTarget = iterTemp;
				}
				iterNext(iterTemp, polytope);
				if (iterTemp == iterStart)
					break;
			}
			iterStart = iterTarget;
			iterEnd = iterTarget;
			iterPrev(iterEnd, polytope);

			//set to begin
			iterTemp = iterStart;
			iterNext(iterTemp, polytope);
			//reset simplex
			info.penetration = minDistance;
			info.simplex.vertices[0] = iterStart->vertex;
			info.simplex.vertices[1] = iterTemp->vertex;

		}
		info.normal = -GeometryAlgorithm2D::pointToLineSegment(info.simplex.vertices[0].result, info.simplex.vertices[1].result
			, { 0, 0 }).normal();
		return info;
	}

	SimplexVertex Narrowphase::support(const ShapePrimitive& shapeA, const ShapePrimitive& shapeB, const Vector2& direction)
	{
		SimplexVertex vertex;
		std::tie(vertex.point[0], vertex.index[0]) = findFurthestPoint(shapeA, direction);
		std::tie(vertex.point[1], vertex.index[1]) = findFurthestPoint(shapeB, direction.negative());
		vertex.result = vertex.point[0] - vertex.point[1];
		return vertex;
	}

	std::pair<Vector2, Index> Narrowphase::findFurthestPoint(const ShapePrimitive& shape, const Vector2& direction)
	{
		Vector2 target;
		Matrix2x2 rot(-shape.transform.rotation);
		Vector2 rot_dir = rot.multiply(direction);
		Index finalIndex = INT_MAX;
		switch (shape.shape->type())
		{
		case Shape::Type::Polygon:
		{
			const Polygon* polygon = static_cast<const Polygon*>(shape.shape);
			std::tie(target, finalIndex) = findFurthestPoint(polygon->vertices(), rot_dir);
			break;
		}
		case Shape::Type::Circle:
		{
			const Circle* circle = static_cast<const Circle*>(shape.shape);
			return std::make_pair(direction.normal() * circle->radius() + shape.transform.position, finalIndex);
		}
		case Shape::Type::Ellipse:
		{
			const Ellipse* ellipse = static_cast<const Ellipse*>(shape.shape);
			target = GeometryAlgorithm2D::calculateEllipseProjectionPoint(ellipse->A(), ellipse->B(), rot_dir);
			break;
		}
		case Shape::Type::Edge:
		{
			const Edge* edge = static_cast<const Edge*>(shape.shape);
			const real dot1 = Vector2::dotProduct(edge->startPoint(), direction);
			const real dot2 = Vector2::dotProduct(edge->endPoint(), direction);
			target = dot1 > dot2 ? edge->startPoint() : edge->endPoint();
			break;
		}
		case Shape::Type::Capsule:
		{
			const Capsule* capsule = static_cast<const Capsule*>(shape.shape);
			target = GeometryAlgorithm2D::calculateCapsuleProjectionPoint(capsule->width(), capsule->height(), rot_dir);
			break;
		}
		case Shape::Type::Sector:
		{
			const Sector* sector = static_cast<const Sector*>(shape.shape);
			target = GeometryAlgorithm2D::calculateSectorProjectionPoint(sector->startRadian(), sector->spanRadian(), sector->radius(), rot_dir);
			break;
		}
		case Shape::Type::Curve:
			assert(false && "Mapping for curve is not supported.");
			break;
		}
		rot.set(shape.transform.rotation);
		target = rot.multiply(target);
		target += shape.transform.position;
		return std::make_pair(target, finalIndex);
	}

	Vector2 Narrowphase::findDirectionByEdge(const SimplexVertex& v1, const SimplexVertex& v2, bool pointToOrigin)

	{
		const Vector2 p1 = v1.result;
		const Vector2 p2 = v2.result;
		const Vector2 ao = p1.negative();
		const Vector2 ab = p2 - p1;
		Vector2 perpendicularOfAB = ab.perpendicular();
		if ((Vector2::dotProduct(ao, perpendicularOfAB) < 0 && pointToOrigin) || (
			Vector2::dotProduct(ao, perpendicularOfAB) > 0 && !pointToOrigin))
			perpendicularOfAB.negate();
		return perpendicularOfAB;
	}
	std::pair<Vector2, Index> Narrowphase::findFurthestPoint(const Container::Vector<Vector2>& vertices, const Vector2& direction)
	{
		real max = Constant::NegativeMin;
		Vector2 target;
		Index index = 0;
		for (Index i = 0; i < vertices.size(); i++)
		{
			real result = Vector2::dotProduct(vertices[i], direction);
			if (max < result)
			{
				max = result;
				target = vertices[i];
				index = i;
			}
		}
		return std::make_pair(target, index);
	}

	ContactPair Narrowphase::generateContacts(const ShapePrimitive& shapeA, 
		const ShapePrimitive& shapeB, CollisionInfo& info)
	{
		ContactPair pair;
		//find feature

		//features[0] is for shapeA, features[1] is for shapeB
		std::array<Feature, 2> features;
		features[0] = findFeatures(info.simplex, info.normal, shapeA, 0);
		features[1] = findFeatures(info.simplex, info.normal, shapeB, 1);

		Shape::Type typeA = shapeA.shape->type();
		Shape::Type typeB = shapeB.shape->type();

		assert(!(typeA == Shape::Type::Edge && typeB == Shape::Type::Edge) && "Not support two edge");
		
		if((typeA == Shape::Type::Polygon || typeA == Shape::Type::Edge) && 
			(typeB == Shape::Type::Polygon || typeB == Shape::Type::Edge))
		{
			Vector2 va1, va2, vb1, vb2;
			if(typeA == Shape::Type::Polygon)
			{
				const Polygon* polygonA = static_cast<const Polygon*>(shapeA.shape);
				va1 = shapeA.transform.translatePoint(polygonA->vertices()[features[0].index[0]]);
				va2 = shapeA.transform.translatePoint(polygonA->vertices()[features[0].index[1]]);
			}
			else
			{
				const Edge* edgeA = static_cast<const Edge*>(shapeA.shape);
				va1 = shapeA.transform.translatePoint(edgeA->startPoint());
				va2 = shapeA.transform.translatePoint(edgeA->endPoint());
			}
			if (typeB == Shape::Type::Polygon)
			{
				const Polygon* polygonB = static_cast<const Polygon*>(shapeB.shape);
				vb1 = shapeB.transform.translatePoint(polygonB->vertices()[features[1].index[0]]);
				vb2 = shapeB.transform.translatePoint(polygonB->vertices()[features[1].index[1]]);
			}
			else
			{
				const Edge* edgeB = static_cast<const Edge*>(shapeB.shape);
				vb1 = shapeB.transform.translatePoint(edgeB->startPoint());
				vb2 = shapeB.transform.translatePoint(edgeB->endPoint());
			}

			std::array<ClipVertex, 2> incEdge;
			std::array<Vector2, 2> refEdge = { va1, va2 };

			Vector2 refNormal = info.normal;

			incEdge[0].vertex = vb1;
			incEdge[1].vertex = vb2;

			const bool swap = Math::abs((va2 - va1).dot(refNormal))
			> Math::abs((vb2 - vb1).dot(refNormal));

			if (swap)
			{
				//swap
				refEdge[0] = vb1;
				refEdge[1] = vb2;
				incEdge[0].vertex = va1;
				incEdge[1].vertex = va2;
				//notice, default normal is changed.
				refNormal.negate();
			}
			pair = clipTwoEdge(incEdge, refEdge, refNormal, swap);

		}
		else if((typeA == Shape::Type::Polygon || typeA == Shape::Type::Edge) && (typeB == Shape::Type::Circle || typeB == Shape::Type::Ellipse))
		{
			Vector2 va1, va2;
			if (typeA == Shape::Type::Polygon)
			{
				const Polygon* polygonA = static_cast<const Polygon*>(shapeA.shape);
				va1 = shapeA.transform.translatePoint(polygonA->vertices()[features[0].index[0]]);
				va2 = shapeA.transform.translatePoint(polygonA->vertices()[features[0].index[1]]);
			}
			else
			{
				const Edge* edgeA = static_cast<const Edge*>(shapeA.shape);
				va1 = shapeA.transform.translatePoint(edgeA->startPoint());
				va2 = shapeA.transform.translatePoint(edgeA->endPoint());
			}
			Vector2 pA = features[1].vertex[0] - info.normal * info.penetration;
			Vector2 edge = va2 - va1;
			edge.normalize();
			real checkZero = edge.dot(info.normal);
			if(Math::abs(checkZero) < 1e-3f)
				pair.addContact(pA, features[1].vertex[0]);
			else
			{
				Vector2 realPa = (pA - va1).lengthSquare() > (pA - va2).lengthSquare() ? va2 : va1;
				pair.addContact(realPa, realPa + info.normal * info.penetration);
			}
		}
		else if ((typeB == Shape::Type::Polygon || typeB == Shape::Type::Edge) && (typeA == Shape::Type::Circle || typeA == Shape::Type::Ellipse))
		{
			Vector2 vb1, vb2;
			if (typeB == Shape::Type::Polygon)
			{
				const Polygon* polygonB = static_cast<const Polygon*>(shapeB.shape);
				vb1 = shapeB.transform.translatePoint(polygonB->vertices()[features[1].index[0]]);
				vb2 = shapeB.transform.translatePoint(polygonB->vertices()[features[1].index[1]]);
			}
			else
			{
				const Edge* edgeB = static_cast<const Edge*>(shapeB.shape);
				vb1 = shapeB.transform.translatePoint(edgeB->startPoint());
				vb2 = shapeB.transform.translatePoint(edgeB->endPoint());
			}
			Vector2 pB = features[0].vertex[0] + info.normal * info.penetration;
			Vector2 edge = vb2 - vb1;
			edge.normalize();
			real checkZero = edge.dot(info.normal);
			if(Math::abs(checkZero) < 1e-3f)
				pair.addContact( features[0].vertex[0], pB);
			else
			{
				Vector2 realPb = (pB - vb1).lengthSquare() > (pB - vb2).lengthSquare() ? vb2 : vb1;
				pair.addContact(realPb - info.normal * info.penetration, realPb);
			}
		}
		else if ((typeA == Shape::Type::Circle || typeA == Shape::Type::Ellipse) && (typeB == Shape::Type::Circle || typeB == Shape::Type::Ellipse))
		{
			const Vector2 v1 = (info.simplex.vertices[0].point[1] - info.simplex.vertices[0].point[0]).normal();
			const Vector2 v2 = (info.simplex.vertices[1].point[1] - info.simplex.vertices[1].point[0]).normal();

			if(v1.dot(info.normal) > v2.dot(info.normal))
				pair.addContact(info.simplex.vertices[0].point[0], info.simplex.vertices[0].point[1]);
			else
				pair.addContact(info.simplex.vertices[1].point[0], info.simplex.vertices[1].point[1]);

			const Vector2 newNormal = pair.points[1] - pair.points[0];
			info.penetration = newNormal.length();
			info.normal = newNormal.normal();
		}
		else
		{
			assert(false && "Not support this type");
		}
		
		return pair;
	}

	real Narrowphase::gjkDistance(const ShapePrimitive& shapeA, const ShapePrimitive& shapeB, const size_t& iteration)
	{
		return real();
	}

	void Narrowphase::sat(const ShapePrimitive& shapeA, const ShapePrimitive& shapeB)
	{

	}
	void Narrowphase::satPolygonVsPolygon(const Polygon& polygonA, const Transform& transformA, const Polygon& polygonB, const Transform& transformB)
	{
	}
	void Narrowphase::satPolygonVsCircle(const Polygon& polygonA, const Transform& transformA, const Circle& circleB, const Transform& transformB)
	{
	}
	void Narrowphase::satPolygonVsEllipse(const Polygon& polygonA, const Transform& transformA, const Ellipse& ellipseB, const Transform& transformB)
	{
	}
	void Narrowphase::satPolygonVsEdge(const Polygon& polygonA, const Transform& transformA, const Edge& edgeB, const Transform& transformB)
	{
	}

	Feature Narrowphase::findFeatures(const Simplex& simplex, const Vector2& normal, const ShapePrimitive& shape, const Index& AorB)
	{
		Feature feature;

		if (shape.shape->type() == Shape::Type::Polygon)
		{
			if (simplex.vertices[0].point[AorB] == simplex.vertices[1].point[AorB])
			{
				//same vertex case
				//find neighbor index

				const Polygon* polygon = static_cast<const Polygon*>(shape.shape);

				const Index tempIndex = simplex.vertices[0].index[AorB];
				const size_t realSize = polygon->vertices().size();
				//TODO: change vertex convention of polygon 
				const Index tempIndexNext = (tempIndex + 1) % realSize;
				const Index tempIndexPrev = (tempIndex - 1 + realSize) % realSize;

				//check most perpendicular
				const Vector2 ab = polygon->vertices()[tempIndexNext] - polygon->vertices()[tempIndex];
				const Vector2 ac = polygon->vertices()[tempIndex] - polygon->vertices()[tempIndexPrev];

				const Vector2 n = shape.transform.inverseRotatePoint(normal);

				const real dot1 = Math::abs(Vector2::dotProduct(ab, n));
				const real dot2 = Math::abs(Vector2::dotProduct(ac, n));

				if (dot1 > dot2)
				{
					feature.index[0] = tempIndexPrev;
					feature.index[1] = tempIndex;
				}
				else
				{
					feature.index[0] = tempIndex;
					feature.index[1] = tempIndexNext;
				}
				
			}
			else
			{
				//polygon edge case
				feature.index[0] = simplex.vertices[0].index[AorB];
				feature.index[1] = simplex.vertices[1].index[AorB];
			}
		}
		else
		{
			feature.vertex[0] = simplex.vertices[0].point[AorB];
			feature.vertex[1] = simplex.vertices[1].point[AorB];
		}
		return feature;
	}
	ContactPair Narrowphase::clipTwoEdge(std::array<ClipVertex, 2>& incEdge, std::array<Vector2, 2> refEdge, const Vector2& normal, bool swap)
	{
		ContactPair pair;
		const Vector2 refEdgeDir = (refEdge[1] - refEdge[0]).normal();
		const Vector2 refEdgeNormal = GeometryAlgorithm2D::lineSegmentNormal(refEdge[0], refEdge[1], normal);

		//check ref1
		const bool isRef1Inc1Valid = refEdgeDir.dot(incEdge[0].vertex - refEdge[0]) >= 0;
		const bool isRef1Inc2Valid = refEdgeDir.dot(incEdge[1].vertex - refEdge[0]) >= 0;

		assert(isRef1Inc1Valid || isRef1Inc2Valid && "Invalid features.");

		if (!isRef1Inc1Valid && isRef1Inc2Valid)
		{
			incEdge[0].isClip = true;
			incEdge[0].vertex = GeometryAlgorithm2D::rayRayIntersectionUnsafe(refEdge[0], refEdgeNormal,
				incEdge[0].vertex, incEdge[1].vertex - incEdge[0].vertex);
			incEdge[0].clipperVertex = refEdge[0];
		}
		else if (isRef1Inc1Valid && !isRef1Inc2Valid)
		{
			incEdge[1].isClip = true;
			incEdge[1].vertex = GeometryAlgorithm2D::rayRayIntersectionUnsafe(refEdge[0], refEdgeNormal,
				incEdge[0].vertex, incEdge[1].vertex - incEdge[0].vertex);
			incEdge[1].clipperVertex = refEdge[0];
		}
		//check ref2
		const bool isRef2Inc1Valid = refEdgeDir.dot(incEdge[0].vertex - refEdge[1]) <= 0;
		const bool isRef2Inc2Valid = refEdgeDir.dot(incEdge[1].vertex - refEdge[1]) <= 0;

		assert(isRef2Inc1Valid || isRef2Inc2Valid && "Invalid features.");

		if (!isRef2Inc1Valid && isRef2Inc2Valid)
		{
			incEdge[0].isClip = true;
			incEdge[0].vertex = GeometryAlgorithm2D::rayRayIntersectionUnsafe(refEdge[1], refEdgeNormal,
				incEdge[0].vertex, incEdge[1].vertex - incEdge[0].vertex);
			incEdge[0].clipperVertex = refEdge[1];
		}
		else if (isRef2Inc1Valid && !isRef2Inc2Valid)
		{
			incEdge[1].isClip = true;
			incEdge[1].vertex = GeometryAlgorithm2D::rayRayIntersectionUnsafe(refEdge[1], refEdgeNormal,
				incEdge[0].vertex, incEdge[1].vertex - incEdge[0].vertex);
			incEdge[1].clipperVertex = refEdge[1];
		}

		//check ref normal region
		incEdge[0].isFinalValid = (incEdge[0].vertex - refEdge[0]).dot(refEdgeNormal) >= 0;
		incEdge[1].isFinalValid = (incEdge[1].vertex - refEdge[0]).dot(refEdgeNormal) >= 0;

		assert(incEdge[0].isFinalValid || incEdge[1].isFinalValid && "Invalid features.");

		if (incEdge[0].isFinalValid && !incEdge[1].isFinalValid)
		{
			//discard invalid, project valid point to segment
			Vector2 incContact1 = incEdge[0].vertex;
			Vector2 refContact1 = incEdge[0].clipperVertex;

			if (!incEdge[0].isClip)
			{
				incContact1 = incEdge[0].vertex;
				refContact1 = GeometryAlgorithm2D::pointToLineSegment(refEdge[0], refEdge[1], incEdge[0].vertex);
			}

			if (swap)
				std::swap(incContact1, refContact1);

			pair.addContact(refContact1, incContact1);

		}
		else if (!incEdge[0].isFinalValid && incEdge[1].isFinalValid)
		{
			//discard invalid, project valid point to segment
			Vector2 incContact2 = incEdge[1].vertex;
			Vector2 refContact2 = incEdge[1].clipperVertex;

			if (!incEdge[1].isClip)
			{
				incContact2 = incEdge[1].vertex;
				refContact2 = GeometryAlgorithm2D::pointToLineSegment(refEdge[0], refEdge[1], incEdge[1].vertex);
			}

			if (swap)
				std::swap(incContact2, refContact2);

			pair.addContact(refContact2, incContact2);
		}
		else
		{
			//both valid, continue to check isClip

			Vector2 incContact1 = incEdge[0].vertex;
			Vector2 refContact1 = incEdge[0].clipperVertex;

			Vector2 incContact2 = incEdge[1].vertex;
			Vector2 refContact2 = incEdge[1].clipperVertex;

			if (!incEdge[0].isClip)
			{
				incContact1 = incEdge[0].vertex;
				refContact1 = GeometryAlgorithm2D::pointToLineSegment(refEdge[0], refEdge[1], incEdge[0].vertex);
			}
			if (!incEdge[1].isClip)
			{
				incContact2 = incEdge[1].vertex;
				refContact2 = GeometryAlgorithm2D::pointToLineSegment(refEdge[0], refEdge[1], incEdge[1].vertex);
			}

			if (swap)
			{
				std::swap(incContact1, refContact1);
				std::swap(incContact2, refContact2);
			}

			pair.addContact(refContact1, incContact1);
			pair.addContact(refContact2, incContact2);
		}
		return pair;
	}

}
