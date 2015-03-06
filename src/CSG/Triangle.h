//
//  Triangle.h
//
//  Created by lars berg on 3/3/15.
//

#pragma once

#include <LineSegment.h>

namespace ofxCSG
{
	class Triangle
	{
	public:
		Triangle(ofVec3f a, ofVec3f b, ofVec3f c) :
		a( a ),
		b( b ),
		c( c ),
		classification( UNDEFINED ),
		normal( normalFromPoints(a, b, c) ),
		w( normal.dot(a) )
		{}
		
		Triangle() :
		classification( UNDEFINED )
		{}
		
		~Triangle()
		{}
		
		ofVec3f* getPtr() {
			return &a;
		}
		
		ofVec3f& operator[]( int n ){
			return getPtr()[n];
		}
		
		void set( ofVec3f _a, ofVec3f _b, ofVec3f _c )
		{
			a = _a;
			b = _b;
			c = _c;
			calcNormal();
		}
		
		void flip()
		{
			swap(b, c);
			normal *= -1;
			w = -w;
			
			if(classification == FRONT)	classification = BACK;
			else if(classification == BACK)	classification = FRONT;
		}
		
		void calcNormal()
		{
			normal = normalFromPoints( a, b, c );
			w = normal.dot( a );
		}
		
		float getArea()
		{
			return areaOfTriangle( a, b, c );
		}
		
		float getAreaSquared()
		{
			return areaOfTriangleSquared( a, b, c );
		}
		
		
		
		//derived from Akira-Hayasaka's ofxRayTriangleIntersection
		//	https://github.com/Akira-Hayasaka/ofxRayTriangleIntersection/blob/master/src/ofxRayTriangleIntersection.h
		//	assume ray direction is normalized
		bool intersectRay( ofVec3f rayOrigin, ofVec3f rayDir, ofVec3f* intersection = NULL )
		{
			float vn = rayDir.dot(normal);
			
			ofVec3f diff = rayOrigin - a;
			float xpn = diff.dot( normal );
			float distance = -xpn / vn;
			
			if (distance < 0) return false; // behind ray origin. fail
			
			ofVec3f hitPos = rayDir * distance + rayOrigin;
			
			if( isPointInTriangle( hitPos, a, b, c, normal ) )
			{
				//it's a hit
				if(intersection!= NULL)
				{
					*intersection = hitPos;
				}
				return true;
			}
			
			//nada
			return false;
		}
		//derived from Akira-Hayasaka's ofxRayTriangleIntersection
		//	https://github.com/Akira-Hayasaka/ofxRayTriangleIntersection/blob/master/src/ofxRayTriangleIntersection.h
		//	assume ray direction is normalized
		bool intersectRay( ofVec3f rayOrigin, ofVec3f rayDir, float epsilon, ofVec3f* intersection = NULL )
		{
			float vn = rayDir.dot(normal);
			
			ofVec3f diff = rayOrigin - a;
			float xpn = diff.dot( normal );
			float distance = -xpn / vn;
			
			if (distance < 0) return false; // behind ray origin. fail
			
			ofVec3f hitPos = rayDir * distance + rayOrigin;
			
			if( isPointInTriangle( hitPos, a, b, c, normal, epsilon ) )
			{
				//it's a hit
				if(intersection!= NULL)
				{
					*intersection = hitPos;
				}
				return true;
			}
			
			//nada
			return false;
		}
		
		
		ofVec3f getCenter()
		{
			return (a + b + c) / 3.;
		}
		
		void draw(bool useNormalForColor = true)
		{
			if(classification == BACK)
			{
				return;
			}
			
			if(useNormalForColor)	ofSetColor( ofFloatColor(normal.x * .5 + .5, normal.y * .5 + .5, normal.z * .5 + .5) );
			ofDrawTriangle( a, b, c );
		}
		
		ofPolyline toPolyline()
		{
			ofPolyline p;
			p.addVertices( getPtr(), 3);
			p.setClosed(true);
			
			return p;
		}
		
		Classification getClassification( ofVec3f planeNormal, float planeW )
		{
			
			int frontCount = 0, backCount = 0;
			for(int i=0; i<3; i++)
			{
				auto c = classifyPointWithPlane( (*this)[i], planeNormal, planeW);
				if(c == FRONT)	frontCount++;
				else if(c == BACK)	backCount++;
			}
			
			if(frontCount && backCount)	return SPANNING;
			else if( backCount )	return BACK;
			else if( frontCount )	return FRONT;
			else return COPLANAR;
		}
		
		void classifyWithPlane( ofVec3f planeNormal, float planeW )
		{
			classification = getClassification( planeNormal, planeW );
		}
		
		vector<LineSegment> getEdges()
		{
			vector<LineSegment> edges(3);
			
			edges[0].set(a, b);
			edges[1].set(b, c);
			edges[2].set(c, a);
			
			return edges;
		}
		
		
		vector<ofVec3f> intersectWithPlane( ofVec3f planeNormal, float planeW )
		{
			vector<ofVec3f> intersections;
			
			ofVec3f intersection;
			
			for(int i=0, j=1; i<3; i++, j++)
			{
				if( splitLineSegmentWithPlane( (*this)[i], (*this)[j%3], planeNormal, planeW, &intersection) )
				{
					intersections.push_back( intersection );
				}
			}
			
			return intersections;
		}
		
		bool getIntersection( Triangle& t, LineSegment* overlap )
		{
			//TODO:: have intersect with plane return a LineSegment
			auto i0 = intersectWithPlane( t.normal, t.w );
			auto i1 = t.intersectWithPlane( normal, w );
			
			
			if(i0.size() < 2 || i1.size() < 2)
			{
				cout << "i0.size() < 2 || i1.size() < 2" << endl;
				return false;
			}
			
			LineSegment l0( i0[0], i0[1] );
			LineSegment l1( i1[0], i1[1] );
			
			for(int i=2;i<i0.size();i++)	l0.expandToPoint( i0[i] );
			for(int i=2;i<i1.size();i++)	l1.expandToPoint( i1[i] );
			
			if( l0.subtract( l1 )  )
			{
				*overlap = l0;
				return true;
			}
			//TODO: I don't think we need this second pass
			else if( l1.subtract( l0 ) )
			{
				cout << "does this ever happen?" << endl;
				*overlap = l1;
				return true;
			}
			
			return false;
		}
		
		vector<Triangle> insert( ofVec3f v )
		{
			vector<Triangle> triangles;
			
			if( abs(distanceToPlaneSigned( v, a, normal) ) > EPSILON)
			{
				triangles.push_back( *this );
				return triangles;
			}
			
			//make three triangles
			for(int i=0; i<3; i++)
			{
				Triangle t( (*this)[i], (*this)[(i+1)%3], v);
				t.classification - classification;
				if(t.getAreaSquared() > EPSILON)
				{
					triangles.push_back(t);
				}
			}
			
			return triangles;
		}
		
		
		bool coplanarWithTrianle(Triangle& t)
		{
			auto nDot = normal.dot( t.normal );
			
			if( abs(nDot) >= 1. && abs( distanceToPlane( a, t.a, t.normal ) ) < EPSILON )
			{
				return true;
			}
			
			return false;
		}
		
		bool doCoplanarTrianglesOverlap(Triangle& t)
		{
			for(int i=0; i<3; i++)
			{
				if( isPointInTriangle( (*this)[i], t.a, t.b, t.c) || isPointInTriangle( t[i], a, b, c) )	return true;
			}
			
			return false;
		}
		
//		vector<Triangle> coplanarSplit(Triangle& t)
//		{
//			vector<Triangle> triangles;
//			//are they coplanar?
//			if( coplanarWithTrianle(t) )//do we need EPSILON here?
//			{
//				if( doPlanarTrianglesOverlap(t) )
//				{
//					auto nDot = normal.dot( t.normal );
//					
//					//subtract or add the triangles from each other and return the new triangles
//					return splitWithCoplanarTriangle( t, nDot );
//				}
//			}
//			
//			//no intersection
//			triangles.push_back( *this );
//			return triangles;
//		}

		vector<Triangle> splitWithCoplanarSegment(ofVec3f a, ofVec3f b)
		{
			return splitWithCoplanarSegment( LineSegment(a, b) );
		}
		
		vector<Triangle> splitWithCoplanarSegment(LineSegment segment)
		{
			vector<Triangle> triangles;
			
			//they intersect, let's split using the overlap segment
			auto firstPass = insert( segment.a );
			
			//	we need to trim the line segment for each triangle because we're inserting the end points
			//	into each triangle to subdivide.
			for(auto& tri: firstPass)
			{
				auto trimedSegment = segment;
				trimedSegment.trimToTriangle( tri.a, tri.b, tri.c );
				
				auto subd = tri.insert( trimedSegment.b );
				if(subd.size() == 1)
				{
					subd = tri.insert( trimedSegment.a );
				}
				
				triangles.insert( triangles.end(), subd.begin(), subd.end() );
			}
			
			if(!triangles.size())
			{
				triangles.push_back( *this );
			}
			
			return triangles;
		}
		
		
		vector<Triangle> split( Triangle& t )
		{
			vector<Triangle> triangles;
			
			auto c = getClassification( t.normal, t.w );
			
			if( c == SPANNING || c == COPLANAR )
			{
				//otherwise check if the other triangle spans this one
				c = t.getClassification(normal, w);
				
				//if they both span then they might intersect. so we'll find the line segment where they overlap
				//	and split it with that segment
				LineSegment overlap;
				if( c == SPANNING && getIntersection( t, &overlap ) )
				{
					return splitWithCoplanarSegment( overlap );
					
					triangles.push_back( *this );
				}
				else
				{
					//no intersection
					triangles.push_back( *this );
				}
			}
			else
			{
				//no intersection
				triangles.push_back( *this );
			}
			
			return triangles;
		}
		
		
		
		ofVec3f a, b, c;
		ofVec3f normal;
		float w;
		Classification classification;
	};
	
	
	//STATIC METHODS
	static vector<Triangle> meshToTriangles(ofMesh& m)
	{
		vector<Triangle> triangles;
		
		auto indices = m.getIndices();
		auto v = m.getVertices();
		
		if(indices.size())
		{
			for(int i=0; i<indices.size(); i+=3)
			{
				triangles.push_back( Triangle( v[ indices[i] ], v[ indices[i+1] ], v[ indices[i+2] ] ) );
			}
		}
		else
		{
			for(int i=0; i<v.size(); i+=3)
			{
				triangles.push_back( Triangle( v[i], v[i+1], v[i+2] ) );
			}
		}
		
		return triangles;
	}
}