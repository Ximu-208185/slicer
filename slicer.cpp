#include "slicer.h"
#include <iostream>
#include <map>
#include <vector>
#include<string>
#include <fstream>
using namespace std;

map<int, vector<vector<float> > > Slicing(Trimesh & m, std::vector<float>& P)
{
	map<int, vector<vector<float> > > segs;
	map<int, vector<unsigned int> > fgroup = Build_triangle_list(P, m);
	map<int,vector<vector<float> > >temp;

	vector<unsigned int> A;
	int N = fgroup.size();
	//int iLayer;
	for (int iLayer = 0; iLayer < N; iLayer++)
	{
		A.insert(A.end(), fgroup[iLayer].begin(), fgroup[iLayer].end());
		vector<vector<float>> S;
		vector<unsigned int> Temp;
		std::cout << iLayer << ": "  <<A.size() << std::endl;
		for (unsigned int i = 0; i < A.size(); i++)
		{
			unsigned int fi = A[i];
			float zmax = m.get_f_maxz(fi);

			if (zmax < P[iLayer])
			{
				Temp.push_back(fi);
			}
			else {
			
				vector<float> seg = ComputeIntersection(fi, P[iLayer], m);
				S.push_back(seg);				
			}
		}
		//To be optimized: remove faces under the current plane.
		for (unsigned int i = 0; i < Temp.size(); i++)
		{
			A.erase(std::remove(A.begin(), A.end(), Temp[i]), A.end());
		}
		segs[iLayer] = S;
		temp= contour_construction(N, S);
	
		//contour_construction(N,S);
	}

	return segs;
}

map<int, vector<unsigned int> > Build_triangle_list(std::vector<float> &P, Trimesh &m)
{
	//int N = P.size() + 1;
	map<int, vector<unsigned int> > face_group_by_layer;
	
	for (unsigned int fi = 0; fi < m.num_F(); fi++)
	{
		unsigned int li = binary_searching(P, fi, m);
		face_group_by_layer[li].push_back(fi);
	}		
	
	return face_group_by_layer;
};

int binary_searching(vector<float> &P, unsigned int fi, Trimesh &m)
{	
	float min_zx = m.get_f_minz(fi);
	int k = P.size();
	if (min_zx > P[k - 1]) return k;
	if (min_zx < P[0]) return 0;
	int low = 0;
	int high = k - 1;
	while (high - low > 1)
	{
		int m = (high + low) / 2;
		if (min_zx > P[m])
			low = m;
		else
			high = m;
	}	
	return high;
}

//https://stackoverflow.com/questions/3142469/determining-the-intersection-of-a-triangle-and-a-plane
std::vector<float> ComputeIntersection(unsigned int fi, float plane, Trimesh &m)
{
	//...
	std::vector<float> seg(6, 0.0f);	
	std::vector<std::vector<float> > upperVerts, downVerts;
	const unsigned int *pF = m.F(fi);
	for (int i = 0; i < 3; i++)
	{
		const unsigned int vi = m.F(fi)[i];
		std::vector<float>vert(4, 0);      //定义顶点的容器，由三个坐标和与切平面z坐标之差描述
		vert[0] = m.V(vi)[0], vert[1] = m.V(vi)[1], vert[2] = m.V(vi)[2];   //三角形三个顶点
		float dis_to_plane = vert[2] - plane;   //确定平面的距离
		if (dis_to_plane > 0)
		{
			vert[3] = dis_to_plane;
			upperVerts.push_back(vert);
		}
		else
		{
			vert[3] = -dis_to_plane;
			downVerts.push_back(vert);
		}
		//  std::vector<float> vert(4,0);       //vert = float[4] x,y,z and z-p
		//vert[0] = m.V(vi)[0], vert[1] = m.V(vi)[1], vert[2] = m.V(vi)[2]
		//float dis_to_plane = vert[2] - plane
		//if dis_to_plane > plane
		//  verts[3] = dis_to_plane
		//  upperVerts.push_back(vert);
		//else
		//  verts[3] = -dis_to_plane;
		//  downVerts.push_back(vert);
		//         
	}
	int k = 0;   
	for(int i = 0; i<upperVerts.size(); i++)
	{
		for (int j = 0; j < downVerts.size(); j++)
		{
			seg[k * 3] = upperVerts[i][0] + (downVerts[j][0] - upperVerts[i][0]) * (plane - upperVerts[i][2]) / (downVerts[j][2] - upperVerts[i][2]);
			seg[k * 3 + 1] = upperVerts[i][1] + (downVerts[j][1] - upperVerts[i][1]) * (plane - upperVerts[i][2]) / (downVerts[j][2] - upperVerts[i][2]);
			seg[k * 3 + 2] = plane;
			
			k++;
			//compute intersection point on upperVerts[i]-downVerts[j] 
			//seg[k*3] = ?
			//seg[k*3+1] = ?
			//seg[k*3+2] = ?
		}
		
	}
	return seg;
}


map<int, vector<vector<float> > >
contour_construction(int q, vector<vector<float> >S)
{
	map<int, vector<vector<float> > > contours;

	multimap<vector<float>, vector<float >>hash;
	vector<vector<float> >temp;                               
	vector<float>points,points_one;     

	//int a = 0;
	int b = 0;
	for (int i = 0; i < S.size(); i++)
	{
		for (int j = 0; j < 3; j++)
		{
			points_one.push_back( S[i][j]);
			points.push_back(S[i][j+3]);
		}
		if (points != points_one&& points[2] == S[0][2] && points_one[2] == S[0][2])
		{
			hash.insert(make_pair(points_one, points));
		}
		points.clear();
		points_one.clear();
	}

	for (int j = 0; j < 3; j++)
	{
		points.push_back(S[0][j]);
	}
	while (!hash.empty())
	{
		multimap<vector<float>, vector<float >>::iterator it1 = hash.find(points);
		if (it1 != hash.end())
		{
			temp.push_back(it1->first);
			temp.push_back(it1->second);
			points = it1->second;
			it1=hash.erase(it1);
			if (points == temp[b] && hash.size() != 0)
			{
				b = temp.size();
				it1 = hash.begin();
				points = it1->second;
				temp.push_back(it1->first);
				temp.push_back(it1->second);
				it1 = hash.erase(it1);
				it1 = hash.end();
			}
			else
			{
				it1 = hash.end();
			}	
		}

		else
		{
			for (auto it1 = hash.begin(); it1 != hash.end();)
			{
				if (it1->second == points)
				{
					points = it1->first;
					temp.push_back(it1->first);
					it1 = hash.erase(it1);
					if (points == temp[b] && hash.size() != 0)
					{
						b = temp.size();
						it1 = hash.begin();
						points = it1->second;
						temp.push_back(it1->first);
						temp.push_back(it1->second);
						it1 = hash.erase(it1);
						it1 = hash.end();
					}
					else
					{
						it1 = hash.end();
					}
				}
				else
				{
					it1++;
				}

			}
		}
	}
			
	std::ofstream out("E:/Project/Slicer/suslicer/b.txt", ios::app);
	for (vector<vector<float>>::iterator qit = temp.begin(); qit != temp.end(); qit++)
	{
		for (vector<float>::iterator pit = (*qit).begin(); pit != (*qit).end(); pit++)
		{
			//cout << *pit << "      ";
			out << to_string(*pit) << "          ";
		}
		//cout << endl;
		out << "\n";
	}
	out.close();
	return contours;
}





	/*for (int i = 0; i < S[0].size()/3; i++)
	{
		for(int j=0;j<S[0].size()/2;j++)
		{
			points.push_back(S[0][a]);
			a++;
		}
		temp.push_back(points);
		if (i==0)
			points.clear();
	}
	for (int i = 1; i < S.size(); i++)
	{
		u.push_back(S[i][0]);
		u.push_back(S[i][1]);
		u.push_back(S[i][2]);        
		v.push_back(S[i][3]);
		v.push_back(S[i][4]);         
		v.push_back(S[i][5]);

		hash1.insert(make_pair(u, v));
		u.clear();
		v.clear();     

	}*/

	//if (S[0][2] == 0.7);
	//	std::ofstream out("C:/Users/Lenovo/Desktop/suslicer/S2.txt", ios::app); 
	//	for (vector<vector<float>>::iterator qit = S.begin(); qit != S.end(); qit++)     
	//	{
	//		for (vector<float>::iterator pit = (*qit).begin(); pit != (*qit).end(); pit++) 
	//		{
	//		    out << to_string(*pit)<< "      ";
	//		}
	//		//cout << endl;
	//		out << "\n";
	//	}
	//	out.close();
	//while (!hash1.empty())     //points != temp[0]
//	{
		/* points[0] = -18.7080345;
		 points[1] = -4.95548534;
		 points[2] = 0.300000012;*/
		//multimap<vector<float>, vector<float >>::iterator it1 = hash1.find(points);
		/*if (points[2] != S[0][2])
		{
			hash1.erase(points);
			it1 = hash1.begin();
			temp.push_back(it1->first);
		}*/
		/* int number;
		 number = hash1.count(points);
		 for (auto it1 = hash1.begin(); it1 != hash1.end();)
			  if (it1->second == points)
				  number++;
			  else
				  it1++;*/
	//	if (it1 != hash1.end())
	//	{
	//		points = it1->second;
	//		it1 = hash1.erase(it1);
	//		temp.push_back(points);
	//		if (points == temp[b] && hash1.size() != 0)
	//		{
	//			b = temp.size();
	//			it1 = hash1.begin();
	//			while ((it1->first)[2] !=S[0][2])
	//			{
	//				it1=hash1.erase(it1);
	//			}
	//			it1 = hash1.begin();
	//			temp.push_back(it1->first);
	//			points = it1->second;
	//			temp.push_back(points);
	//			it1 = hash1.erase(it1);
	//			it1 = hash1.end();
	//		}
	//		else
	//		{
	//			it1 = hash1.end();
	//		}
	//	}
	//	else
	//	{
	//		for (auto it1 = hash1.begin(); it1 != hash1.end();)
	//		{
	//			if (it1->second == points)
	//			{
	//				points = it1->first;
	//				it1 = hash1.erase(it1);
	//				//it1 = hash1.end();
	//				temp.push_back(points);
	//				if (points == temp[b] && hash1.size() != 0)
	//				{
	//					b = temp.size();
	//					it1 = hash1.begin();
	//					while ((it1->first)[2] != S[0][2])
	//					{
	//						it1 = hash1.erase(it1);
	//					}
	//					it1 = hash1.begin();
	//					temp.push_back(it1->first);
	//					points = it1->second;
	//					temp.push_back(points);
	//					it1 = hash1.erase(it1);
	//					it1 = hash1.end();
	//				}
	//				else
	//				{
	//					it1 = hash1.end();
	//				}
	//			}
	//			else
	//			{
	//				it1++;
	//			}
	//		}
	//	}
	//}
	











		

    
