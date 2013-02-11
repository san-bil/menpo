#include <iostream>
#include <ostream>
#include <assert.h>
#include "vertex.h"
#include "halfedge.h"
#include "vec3.h"
#include "triangle.h"

Vertex::Vertex(Mesh* meshIn, unsigned vertid, double* coordsIn): MeshAttribute(meshIn)
{
  id = vertid;
  coords = coordsIn;
}

void Vertex::addTriangle(Triangle* triangle)
{
  triangles.insert(triangle);
}

void Vertex::addVertex(Vertex* vertex)
{
  //std::cout << "V:" << this << " is now connected to V:" << vertex << std::endl;
  vertices.insert(vertex);
}

// returns the created half edge so it can be attached to the triangle if so desired
HalfEdge* Vertex::addHalfEdgeTo(Vertex* vertex, Triangle* triangle)
{
  if(getHalfEdgeTo(vertex) == NULL)
  {
    HalfEdge* halfedge = new HalfEdge(this->mesh,this,vertex,triangle);
    halfedges.insert(halfedge);
    //std::cout << "V:" << this << " is now connected to HE:" << halfedge << std::endl;
    return halfedge;
  }
  else
  {
    std::cout << "This vertex seems to already be connected! Doing nothing." << std::endl;
    return NULL;
  }
}

HalfEdge* Vertex::getHalfEdgeTo(Vertex* vertex)
{
  std::set<HalfEdge*>::iterator he;
  for(he = halfedges.begin(); he != halfedges.end(); he++)
  {
    if((*he)->v1 == vertex)
    {
      //std::cout << "V:" << this << " has a HE to V:" << vertex << std::endl;
      return *he;
    }
  }
  //std::cout << "V:" << this << " does not have a HE to V:" << vertex << std::endl;
  return NULL;
}

Vertex::~Vertex()
{
  halfedges.clear();
}

Vec3 Vertex::operator-(Vertex v)
{
  Vec3 a = *this;
  Vec3 b = v;
  return a - b;
}

std::ostream& operator<<(std::ostream& out, const Vertex& v)
{
  out << "V:" << v.id << " (" << v.coords[0] << "," 
    << v.coords[1] << "," << v.coords[2] << ")";
  return out;
}

HalfEdge* Vertex::halfEdgeOnTriangle(Triangle* triangle)
{
  std::set<HalfEdge*>::iterator he;
  for(he = halfedges.begin(); he != halfedges.end(); he++)
  {
    if((*he)->triangle == triangle)
    {
      //std::cout << "V:" << this << " has a HE to V:" << vertex << std::endl;
      return *he;
    }
  }
  return NULL;
  //std::cout << "V:" << this << " does not have a HE to V:" << vertex << std::endl;
}

void Vertex::calculateLaplacianOperator(unsigned* i_sparse, unsigned* j_sparse,
    double* w_sparse, unsigned& sparse_pointer, 
    double* inv_d_ij_array, LaplacianWeightType weight_type)
{
  //std::cout << "Calculating Laplacian for vertex no. " << id << "(" << halfedges.size() << " halfedges)" << std::endl ;
  // sparse_pointer points into how far into the sparse_matrix structures
  // we should be recording results for this vertex
  //bool has_a_full_edge = false;
  unsigned i = id;
  // the normalisation factor that will be built as we iterate over each half edge
  double inv_d_ij = 0.;
  std::set<HalfEdge*>::iterator he;
  for(he = halfedges.begin(); he != halfedges.end(); he++)
  {
    unsigned j = (*he)->v1->id;
    //std::cout << "j = " << j << std::endl;

    // always calculate the area for this vertex
    inv_d_ij += (*he)->triangle->area();
    //if((*he)->partOfFullEdge())
    //  has_a_full_edge = true;
    //std::cout << *this << " halfedge to " << *((*he)->v1) << std::endl;

    if(i < j)
    {
      //std::cout << "Not breaking as " << i << " >=  " << j << std::endl;
      double w_ij;
      switch(weight_type)
      {
        case cotangent:
          w_ij = cotWeight(*he);
          break;
        case distance:
          w_ij = distanceWeight(*he);
          break;
        case combinatorial:
          w_ij = combinatorialWeight(*he);
          break;
        default:
          std::cout << "I don't know how to calcuate Laplacian weights of this type! " << std::endl;
      }
      //std::cout << "writing out to i:" << i << " j:" << j << " (sparseP=" << sparse_pointer << ")" << std::endl << std::endl;
      // - cotOp to the i,j'th position 
      i_sparse[sparse_pointer] = i;
      j_sparse[sparse_pointer] = j;
      w_sparse[sparse_pointer] = -w_ij;
      // should be only entry here...
      //if(w_sparse[sparse_pointer] != 0)
      //  std::cout << "this matrix value is already taken?" << std::endl;
      //
      // and record the other way for free
      sparse_pointer++;
      j_sparse[sparse_pointer] = i;
      i_sparse[sparse_pointer] = j;
      w_sparse[sparse_pointer] = -w_ij;
      sparse_pointer++;
      // += cotOp to the i'th\'th position twice (for both times)
      w_sparse[i] += w_ij;
      w_sparse[j] += w_ij;
    }
    // else:no point calculating this point - as we know the Laplacian is symmetrical 
    // just rebuild the other side of the matrix later
    //else 
    //  std::cout << "Breaking as " << i << " <  " << j << std::endl;
  }    
  // now we've looped through store the areas in the array that is passed in
  inv_d_ij_array[id] = inv_d_ij/3.0; 
  //if(!has_a_full_edge)
  //  std::cout << "Vertex " << id << " does not have any full edges around it (" << halfedges.size() << " halfedges around it)" << std::endl;
}

double Vertex::cotWeight(HalfEdge* he)
{
  //std::cout << "theta = " << he->gammaAngle();
  double cotOp = cotOfAngle(he->gammaAngle());
  //std::cout << "cot of first angle is " << cotOp << std::endl;
  if(he->partOfFullEdge())
  {
    cotOp += cotOfAngle(he->halfedge->gammaAngle());
    //std::cout << *this << " is a fulledge! After adding second cot, cot of both is " << cotOp << std::endl;
  }
  return cotOp/2.0;
}

double Vertex::distanceWeight(HalfEdge* he)
{
  double length = he->length();
  return 1.0/(length*length);
}

double Vertex::combinatorialWeight(HalfEdge* he)
{
  return 1;
}

void Vertex::divergence(double* t_vector_field, double* v_scalar_divergence)
{
  //std::cout << "Calculating diergence for vertex no. " << id << "(" << halfedges.size() << " halfedges)" << std::endl ;
  std::set<HalfEdge*>::iterator he;
  double divergence = 0;
  for(he = halfedges.begin(); he != halfedges.end(); he++)
  {
    Vec3 field(&t_vector_field[((*he)->triangle->id)*3]);
    //std::cout << "field = " << field << std::endl;
    Vec3 e1 = (*he)->differenceVec3();
    //std::cout << "Got diff vec!" << std::endl;
    // *-1 as we want to reverse the direction
    Vec3 e2 = (*he)->clockwiseAroundTriangle()->clockwiseAroundTriangle()->differenceVec3()*-1;
    //std::cout << "Got other diff vec!" << std::endl;
    double cottheta2 = cotOfAngle((*he)->betaAngle());
    double cottheta1 = cotOfAngle((*he)->gammaAngle());
    //std::cout << "cottheta1 = " << cottheta1 << " cottheta2 = " << cottheta2 << std::endl;
    divergence += cottheta1*(e1.dot(field)) + cottheta2*(e2.dot(field));
  }
  //std::cout << "       divergence is " << divergence/2.0 << std::endl << std::endl;
  v_scalar_divergence[id] = divergence/2.0;
}

void Vertex::verifyHalfEdgeConnectivity()
{
  std::set<HalfEdge*>::iterator he;
  for(he = halfedges.begin(); he != halfedges.end(); he++)
  {
    Triangle* triangle = (*he)->triangle;
    Vertex* t_v0 = triangle->v0;
    Vertex* t_v1 = triangle->v1;
    Vertex* t_v2 = triangle->v2;
    if(t_v0 != this && t_v1 != this && t_v2 != this)
      std::cout << "this halfedge does not live on it's triangle!" << std::endl;
    if((*he)->v0 != this)
      std::cout << "half edge errornously connected" << std::endl;
    if((*he)->clockwiseAroundTriangle()->clockwiseAroundTriangle()->v1 != (*he)->v0)
      std::cout << "cannie spin raarnd the triangle like man!" << std::endl;
    if((*he)->partOfFullEdge())
    {
      if((*he)->halfedge->v0 != (*he)->v1 || (*he)->halfedge->v1 != (*he)->v0)
        std::cout << "some half edges aren't paired up with there buddies!" << std::endl;
    }
  }
}

void Vertex::printStatus()
{
  std::cout << "V" << id << std::endl;
  std::set<HalfEdge*>::iterator he;
  for(he = halfedges.begin(); he != halfedges.end(); he++)
  {
    std::cout << "|" ;
    if((*he)->partOfFullEdge())
      std::cout << "=";
    else
      std::cout << "-";
    std::cout << "V" << (*he)->v1->id;
    std::cout << " (T" << (*he)->triangle->id; 
    if((*he)->partOfFullEdge())
      std::cout << "=T" << (*he)->halfedge->triangle->id;
    std::cout << ")" << std::endl;
  }
}
