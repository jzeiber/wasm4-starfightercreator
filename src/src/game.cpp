#include "game.h"
#include "wasm4.h"
#include "global.h"
#include "palette.h"
#include "textprinter.h"
#include "outputstringstream.h"
#include "font5x7.h"
#include "randommt.h"
#include "cppfuncs.h"
#include "wasmmath.h"
#include "base64.h"

Game::Game():m_ticks(0)
{
	for(int i=0; i<countof(m_points); i++)
	{
		m_points[i].m_used=false;
	}
	m_mode=MODE_ADDPOINT;
	m_selectedpoint=-1;
	m_weaponpoint=-1;
}

Game::~Game()
{

}

void Game::Update(const int ticks, Game *game)
{
	m_ticks+=ticks;
}

uint64_t Game::GetTicks() const
{
	return m_ticks;
}

void Game::Draw()
{
	constexpr uint32_t points=8;
	constexpr uint32_t scale=64/points;
	constexpr uint32_t xoffset=scale/2;
	constexpr uint32_t yoffset=scale/2;
	constexpr uint8_t xpoints[]={1,4,5,6,7,7,7,8,7,7,7,6,5,4,1};

	*DRAW_COLORS=PALETTE_DARKGREY;
	for(int y=0; y<15; y++)
	{
		for(int x=0; x<15; x++)
		{
			int32_t xpos;
			int32_t ypos;
			if(GridPositionToScreenCoord(x,y,xpos,ypos)==true)
			{
				line(xpos,ypos,xpos,ypos);
			}
		}
	}

	*DRAW_COLORS=PALETTE_DARKGREY << 4;
	oval(xoffset,yoffset,14*scale,14*scale);

	point lastpoint;
	lastpoint.m_used=false;
	for(int i=0; i<countof(m_points); i++)
	{
		if(m_points[i].m_used==true)
		{
			if(lastpoint.m_used==true)
			{
				GridLine(lastpoint.m_xidx,lastpoint.m_yidx,m_points[i].m_xidx,m_points[i].m_yidx,true);
			}
			lastpoint=m_points[i];
		}
	}
	for(int i=countof(m_points)-1; i>=0; i--)
	{
		if(m_points[i].m_used==true)
		{
			if(lastpoint.m_used==true)
			{
				point p2=m_points[i].ReflectX();
				GridLine(lastpoint.m_xidx,lastpoint.m_yidx,p2.m_xidx,p2.m_yidx,true);
			}
			lastpoint=m_points[i].ReflectX();
		}
	}
	if(lastpoint.m_used==true)
	{
		point p2=lastpoint.ReflectX();
		GridLine(lastpoint.m_xidx,lastpoint.m_yidx,p2.m_xidx,p2.m_yidx,true);
	}
	// draw weapons
	if(m_weaponpoint>=0 && m_weaponpoint<countof(m_points) && m_points[m_weaponpoint].m_used==true)
	{
		int32_t x1;
		int32_t y1;
		int32_t x2;
		int32_t y2;
		point p2=m_points[m_weaponpoint].ReflectX();
		*DRAW_COLORS=PALETTE_DARKGREY;
		GridPositionToScreenCoord(m_points[m_weaponpoint].m_xidx,m_points[m_weaponpoint].m_yidx,x1,y1);
		GridPositionToScreenCoord(p2.m_xidx,p2.m_yidx,x2,y2);
		line(x1,y1,x1,y1-5);
		line(x2,y2,x2,y2-5);
	}
	// draw selected point
	if(m_selectedpoint>=0 && m_selectedpoint<countof(m_points) && m_points[m_selectedpoint].m_used==true)
	{
		*DRAW_COLORS=PALETTE_WHITE << 4;
		GridBox(m_points[m_selectedpoint].m_xidx,m_points[m_selectedpoint].m_yidx,1);
		point p2=m_points[m_selectedpoint].ReflectX();
		GridBox(p2.m_xidx,p2.m_yidx,1);
	}

	// mouse grid box
	uint32_t xi;
	uint32_t yi;
	if(ScreenCoordToGridPosition(*MOUSE_X,*MOUSE_Y,xi,yi)==true)
	{
		int32_t x;
		int32_t y;
		*DRAW_COLORS=PALETTE_WHITE << 4;
		GridPositionToScreenCoord(xi,yi,x,y);
		rect(x-1,y-1,3,3);
	}

	// draw buttons

	DrawButton(0,"Add","Vertex",m_mode==MODE_ADDPOINT);
	DrawButton(1,"Delete","Vertex",m_mode==MODE_DELETEPOINT);
	DrawButton(2,"Insert","Vertex",m_mode==MODE_INSERTPOINT);
	DrawButton(3,"Select","Vertex",m_mode==MODE_SELECTPOINT);
	DrawButton(4,"Move","Vertex",m_mode==MODE_MOVEPOINT);
	DrawButton(5,"Weapon","Vertex",m_mode==MODE_ADDWEAPONS);

	// draw points used

	OutputStringStream ostr;
	ostr << UsedPoints() << " / " << (uint32_t)countof(m_points);
	TextPrinter tp;
	tp.SetCustomFont(&Font5x7::Instance());
	tp.PrintCentered(ostr.Buffer(),SCREEN_SIZE-20,SCREEN_SIZE-12,10,PALETTE_WHITE);

	// draw small ship previews
	DrawShip(12,12,1.4,0);
	DrawShip(SCREEN_SIZE-40-12,12,1.4,static_cast<float>(m_ticks)/60.0);

	// 
	{
		OutputStringStream enc;
		OutputStringStream err;
		if(ValidateShip(enc,err)==true)
		{
			tp.PrintWrapped("Ship Security Code",1,SCREEN_SIZE-40,18,SCREEN_SIZE-40,PALETTE_WHITE,TextPrinter::JUSTIFY_LEFT);
			tp.PrintWrapped(enc.Buffer(),1,SCREEN_SIZE-30,enc.TextLength(),SCREEN_SIZE-40,PALETTE_WHITE,TextPrinter::JUSTIFY_LEFT);
		}
		else
		{
			tp.PrintWrapped(err.Buffer(),1,SCREEN_SIZE-40,err.TextLength(),SCREEN_SIZE-40,PALETTE_WHITE,TextPrinter::JUSTIFY_LEFT);
		}
	}

}

bool Game::HandleInput(const Input *input)
{
	bool handled=false;

	bool gridclick=false;
	uint32_t gridx;
	uint32_t gridy;
	uint32_t gridaction=m_mode;

	if(input->MouseButtonClick(MOUSE_LEFT)==true)
	{
		if(input->MouseX()>=SCREEN_SIZE-40)
		{
			int32_t button=(input->MouseY()/24)+1;
			if(button>=1 && button<MODE_MAX)
			{
				m_mode=button;
			}
		}

		if(ScreenCoordToGridPosition(*MOUSE_X,*MOUSE_Y,gridx,gridy)==true)
		{
			gridclick=true;
		}
	}

	if(input->MouseButtonClick(MOUSE_RIGHT)==true)
	{
		if(ScreenCoordToGridPosition(*MOUSE_X,*MOUSE_Y,gridx,gridy)==true)
		{
			gridaction=MODE_DELETEPOINT;
			gridclick=true;
		}
	}

	if(input->MouseButtonClick(MOUSE_MIDDLE)==true)
	{
		if(ScreenCoordToGridPosition(*MOUSE_X,*MOUSE_Y,gridx,gridy)==true)
		{
			gridaction=MODE_SELECTPOINT;
			gridclick=true;
		}
	}

	if(gridclick==true)
	{
		switch(gridaction)
		{
		case MODE_ADDPOINT:
			AddPoint(gridx,gridy);
			break;
		case MODE_DELETEPOINT:
			RemovePoint(gridx,gridy);
			break;
		case MODE_INSERTPOINT:
			InsertPoint(gridx,gridy);
			break;
		case MODE_SELECTPOINT:
			SelectPoint(gridx,gridy);
			break;
		case MODE_MOVEPOINT:
			MovePoint(gridx,gridy);
			break;
		case MODE_ADDWEAPONS:
			WeaponsPoint(gridx,gridy);
			break;
		}
	}

	return handled;
}

bool Game::GridPositionToScreenCoord(const uint32_t xidx, const uint32_t yidx, int32_t &x, int32_t &y) const
{
	static constexpr uint32_t points=8;
	static constexpr uint32_t scale=64/points;
	static constexpr uint32_t xoffset=scale/2;
	static constexpr uint32_t yoffset=scale/2;
	static constexpr uint8_t xpoints[]={1,4,5,6,7,7,7,8,7,7,7,6,5,4,1};

	if(yidx<0 || yidx>15)
	{
		return false;
	}
	if(xidx<8-xpoints[yidx] || xidx>=15-(8-xpoints[yidx]))
	{
		return false;
	}

	x=(xidx*scale)+xoffset;
	y=(yidx*scale)+yoffset;

	return true;

}

bool Game::ScreenCoordToGridPosition(const int32_t x, const int32_t y, uint32_t &xidx, uint32_t &yidx) const
{
	static constexpr uint32_t points=8;
	static constexpr uint32_t scale=64/points;
	static constexpr uint32_t xoffset=scale/2;
	static constexpr uint32_t yoffset=scale/2;
	static constexpr uint8_t xpoints[]={1,4,5,6,7,7,7,8,7,7,7,6,5,4,1};

	int32_t xi=(x-xoffset+(scale/2))/scale;
	int32_t yi=(y-yoffset+(scale/2))/scale;

	if(xi>=0 && xi<15 && yi>=0 && yi<15 && xi>=8-xpoints[yi] && xi<15-(8-xpoints[yi]))
	{
		xidx=xi;
		yidx=yi;
		return true;
	}
	return false;
}

bool Game::AddPoint(const uint32_t xidx, const uint32_t yidx)
{
	const uint32_t x=(xidx<7 ? 7+(7-xidx) : xidx);
	const uint32_t y=yidx;
	for(int i=0; i<countof(m_points); i++)
	{
		if(m_points[i].m_used==false)
		{
			m_points[i].m_xidx=x;
			m_points[i].m_yidx=y;
			m_points[i].m_used=true;
			m_selectedpoint=i;
			return true;
		}
	}
	return false;
}

bool Game::RemovePoint(const uint32_t xidx, const uint32_t yidx)
{
	const uint32_t x=(xidx<7 ? 7+(7-xidx) : xidx);
	const uint32_t y=yidx;
	for(int i=0; i<countof(m_points); i++)
	{
		if(m_points[i].m_used==true && m_points[i].m_xidx==x && m_points[i].m_yidx==y)
		{
			m_points[i].m_used=false;
			if(m_selectedpoint==i)
			{
				m_selectedpoint=-1;
			}
			else if(m_selectedpoint>i)
			{
				m_selectedpoint--;
			}
			if(m_weaponpoint==i)
			{
				m_weaponpoint=-1;
			}
			else if(m_weaponpoint>i)
			{
				m_weaponpoint--;
			}
			for(int j=i; j<countof(m_points)-1; j++)
			{
				m_points[j]=m_points[j+1];
				m_points[j+1].m_used=false;
			}
			return true;
		}
	}
	return false;
}

bool Game::SelectPoint(const uint32_t xidx, const uint32_t yidx)
{
	for(int i=0; i<countof(m_points); i++)
	{
		point p2=m_points[i].ReflectX();
		if(m_points[i].m_used==true && ((m_points[i].m_xidx==xidx && m_points[i].m_yidx==yidx) || (p2.m_xidx==xidx && p2.m_yidx==yidx)))
		{
			m_selectedpoint=i;
			return true;
		}
	}
	return false;
}

bool Game::InsertPoint(const uint32_t xidx, const uint32_t yidx)
{
	if(m_selectedpoint>=0 && m_selectedpoint<(countof(m_points)-1))
	{

		point newpoint;
		newpoint.m_used=true;
		newpoint.m_xidx=xidx;
		newpoint.m_yidx=yidx;
		if(xidx<7)
		{
			newpoint=newpoint.ReflectX();
		}

		for(int i=countof(m_points)-1; i>m_selectedpoint+1; i--)
		{
			m_points[i]=m_points[i-1];
		}
		m_points[m_selectedpoint+1]=newpoint;
		m_selectedpoint++;
		if(m_weaponpoint>m_selectedpoint)
		{
			m_weaponpoint++;
		}
	}
	return false;
}

bool Game::MovePoint(const uint32_t xidx, const uint32_t yidx)
{
	if(m_selectedpoint>=0 && m_selectedpoint<countof(m_points) && m_points[m_selectedpoint].m_used==true)
	{
		point newpoint;
		newpoint.m_used=true;
		newpoint.m_xidx=xidx;
		newpoint.m_yidx=yidx;
		if(xidx<7)
		{
			newpoint=newpoint.ReflectX();
		}

		m_points[m_selectedpoint]=newpoint;
		return true;
	}
	return false;
}

bool Game::WeaponsPoint(const uint32_t xidx, const uint32_t yidx)
{
	point p2;
	p2.m_xidx=xidx;
	p2.m_yidx=yidx;
	if(xidx<7)
	{
		p2=p2.ReflectX();
	}
	for(int i=0; i<countof(m_points); i++)
	{
		if(m_points[i].m_used==true && m_points[i].m_xidx==p2.m_xidx && m_points[i].m_yidx==p2.m_yidx)
		{
			m_weaponpoint=i;
			return true;
		}
	}
	return false;
}

void Game::GridLine(const uint32_t xidx1, const uint32_t yidx1, const uint32_t xidx2, const uint32_t yidx2, const bool withpoints) const
{
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
	if(GridPositionToScreenCoord(xidx1,yidx1,x1,y1)==true && GridPositionToScreenCoord(xidx2,yidx2,x2,y2)==true)
	{
		*DRAW_COLORS=PALETTE_LIGHTGREY;
		line(x1,y1,x2,y2);
		if(withpoints==true)
		{
			*DRAW_COLORS=PALETTE_DARKGREY << 4;
			rect(x1-1,y1-1,3,3);
			rect(x2-1,y2-1,3,3);
		}
	}
}

void Game::GridBox(const uint32_t xidx, const uint32_t yidx, const uint32_t size) const
{
	int32_t x1;
	int32_t y1;
	if(GridPositionToScreenCoord(xidx,yidx,x1,y1)==true)
	{
		rect(x1-size,y1-size,1+(size*2),1+(size*2));
	}
}

void Game::DrawButton(const uint8_t num, const char *text1, const char *text2, const bool selected) const
{

	TextPrinter tp;
	tp.SetCustomFont(&Font5x7::Instance());

	*DRAW_COLORS=(selected==true ? (PALETTE_WHITE << 4) : (PALETTE_DARKGREY << 4));
	rect(SCREEN_SIZE-40,num*24,40,24);

	uint32_t yoffset=4+(text2 ? 0 : 4);
	if(text1)
	{
		tp.PrintCentered(text1,SCREEN_SIZE-20,(num*24)+yoffset,32,(selected==true ? PALETTE_WHITE : PALETTE_LIGHTGREY));
	}
	if(text2)
	{
		tp.PrintCentered(text2,SCREEN_SIZE-20,(num*24)+yoffset+8,32,(selected==true ? PALETTE_WHITE : PALETTE_LIGHTGREY));
	}

}

uint8_t Game::UsedPoints() const
{
	uint8_t count=0;
	for(int i=0; i<countof(m_points); i++)
	{
		if(m_points[i].m_used==true)
		{
			count++;
		}
	}
	return count;
}

void Game::DrawShip(const int32_t x, const int32_t y, const float scale, const float rotrad) const
{
	float rad=rotrad;
	while(rad<0)
	{
		rad+=M_PI*2.0;
	}
	while(rad>(M_PI*2.0))
	{
		rad-=(M_PI*2.0);
	}
	struct fpoint
	{
		bool used=false;
		float x;
		float y;
	};
	fpoint points[countof(m_points)*2];
	for(int i=0; i<countof(m_points); i++)
	{
		points[i].used=false;
	}
	int p=0;
	for(int i=0; i<countof(m_points); i++)
	{
		if(m_points[i].m_used==true)
		{
			float sr=_sin(rad);
			float cr=_cos(rad);
			float xcos=static_cast<float>(m_points[i].m_xidx-7)*cr;
			float xsin=static_cast<float>(m_points[i].m_xidx-7)*sr;
			float ycos=static_cast<float>(m_points[i].m_yidx-7)*cr;
			float ysin=static_cast<float>(m_points[i].m_yidx-7)*sr;

			points[p].used=true;
			points[p].x=static_cast<float>(x)+(scale*(xcos-ysin));
			points[p].y=static_cast<float>(y)+(scale*(xsin+ycos));
			p++;
		}
	}
	for(int i=countof(m_points)-1; i>=0; i--)
	{
		if(m_points[i].m_used==true)
		{
			point p2=m_points[i].ReflectX();
			float sr=_sin(rad);
			float cr=_cos(rad);
			float xcos=static_cast<float>(p2.m_xidx-7)*cr;
			float xsin=static_cast<float>(p2.m_xidx-7)*sr;
			float ycos=static_cast<float>(p2.m_yidx-7)*cr;
			float ysin=static_cast<float>(p2.m_yidx-7)*sr;

			points[p].used=true;
			points[p].x=static_cast<float>(x)+(scale*(xcos-ysin));
			points[p].y=static_cast<float>(y)+(scale*(xsin+ycos));
			p++;
		}
	}

	*DRAW_COLORS=PALETTE_WHITE;
	int lastused=-1;
	for(int i=1; i<countof(m_points)*2; i++)
	{
		if(points[i].used==true)
		{
			line(points[i-1].x,points[i-1].y,points[i].x,points[i].y);
			lastused=i;
		}
	}
	if(lastused>-1 && points[0].used==true)
	{
		line(points[0].x,points[0].y,points[lastused].x,points[lastused].y);
	}

}

bool Game::ValidateShip(OutputStringStream &encoded, OutputStringStream &err) const
{
	point unique[3];

	for(int i=0; i<3; i++)
	{
		unique[i].m_used=false;
	}
	for(int i=0; i<countof(m_points); i++)
	{
		if(m_points[i].m_used==true)
		{
			bool isunique=true;
			for(int u=0; u<countof(unique) && isunique==true; u++)
			{
				if(unique[u].m_used==true && unique[u].m_xidx==m_points[i].m_xidx && unique[u].m_yidx==m_points[i].m_yidx)
				{
					isunique=false;
				}
			}
			if(isunique==true)
			{
				for(int j=0; j<countof(unique); j++)
				{
					if(unique[j].m_used==false)
					{
						unique[j]=m_points[i];
						j=countof(unique);
					}
				}
			}
		}
	}
	for(int i=0; i<3; i++)
	{
		if(unique[i].m_used==false)
		{
			err << "Not enough vertices";
			return false;
		}
	}

	if(m_weaponpoint<-1 || m_weaponpoint>countof(m_points) || m_points[m_weaponpoint].m_used==false)
	{
		err << "No weapon vertex";
		return false;
	}

	EncodeShip(encoded);

	return true;

}

bool Game::EncodeShip(OutputStringStream &encoded) const
{

	uint8_t inbytes[12]={0,0,0,0,0,0,0,0,0,0,0,0};
	uint8_t outbytes[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	inbytes[0]=0x01;	// version
	if(m_weaponpoint>=-1 && m_weaponpoint<countof(m_points) && m_points[m_weaponpoint].m_used==true)
	{
		inbytes[1]=m_weaponpoint & 0xf;
	}
	for(int i=0; i<countof(m_points); i++)
	{
		inbytes[i+2]=0;
		point p=m_points[i];
		if(p.m_used==true)
		{
			point p=m_points[i];
			if(p.m_xidx>7)
			{
				p=p.ReflectX();
			}
			inbytes[i+2]|=(1 << 7);
			inbytes[i+2]|=((p.m_xidx & 0x7) << 4);
			inbytes[i+2]|=(p.m_yidx & 0xf);
		}
	}

	base64_encode(inbytes,countof(inbytes),outbytes);

	for(int i=0; i<countof(outbytes); i++)
	{
		encoded << (char)outbytes[i];
	}

	return true;

}
