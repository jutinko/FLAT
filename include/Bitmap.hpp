#ifndef BITMAP_HPP
#define BITMAP_HPP

#ifdef WIN32
#define cimg_display 2
#define cimg_verbosity 3
#else
#define cimg_display 0
#define cimg_verbosity 0
#endif

#include "CImg.h"
#include "Box.hpp"

using namespace std;

namespace FLAT
{
	enum ColorType
	{
		WHITE_COLOR,
		BLACK_COLOR,
		RED_COLOR,
		GREEN_COLOR,
		BLUE_COLOR,
		YELLOW_COLOR,
		GREY_COLOR
	};

	class Color
	{
	public:
		unsigned char rgb[3];
		Color(ColorType c);
		Color(unsigned char R,unsigned char G,unsigned char B);
	};

	class Bitmap
	{
	public:
		cimg_library::CImg<unsigned char> bitmap;
		Box domain;
		Vertex domainExtent;
		Vertex resolution;
		int dimension;
		int channel;

		Bitmap (int dim,int channel,const spaceUnit res,Box& dom,const Color& c);
		Bitmap (string filename);
		Bitmap ();
		~Bitmap();

		void drawLine(const Vertex& l1,const Vertex & l2,const Color& c);
		void drawArrow(const Vertex& l1,const Vertex & l2,const Color& c);
		void drawPoint(const Vertex& p,const Color& c);
		void drawRect(const Box& b,const Color& c);
		void drawFilledRect(const Box& b,const Color& c);
		void drawCircle(const Vertex& center, spaceUnit radius, const Color& c);

		void save(string filename);
		void show();
	};
}

#endif
