#pragma once

#include <stdint.h>

#include "global.h"
#include "istate.h"
#include "iupdatable.h"
#include "idrawable.h"
#include "iinputhandler.h"
#include "outputstringstream.h"

class Game:public IUpdatable,public IDrawable,public IInputHandler
{
public:
	Game();
	~Game();
	
	bool HandleInput(const Input *input);
	void Update(const int ticks, Game *game=nullptr);
	void Draw();

	uint64_t GetTicks() const;

	enum ActionMode
	{
	MODE_NONE=0,
	MODE_ADDPOINT=1,
	MODE_DELETEPOINT=2,
	MODE_INSERTPOINT=3,
	MODE_SELECTPOINT=4,
	MODE_MOVEPOINT=5,
	MODE_ADDWEAPONS=6,
	MODE_MAX
	};

private:

	bool AddPoint(const uint32_t xidx, const uint32_t yidx);
	bool RemovePoint(const uint32_t xidx, const uint32_t yidx);
	bool SelectPoint(const uint32_t xidx, const uint32_t yidx);
	bool InsertPoint(const uint32_t xidx, const uint32_t yidx);
	bool MovePoint(const uint32_t xidx, const uint32_t yidx);
	bool WeaponsPoint(const uint32_t xidx, const uint32_t yidx);
	uint8_t UsedPoints() const;

	bool GridPositionToScreenCoord(const uint32_t xidx, const uint32_t yidx, int32_t &x, int32_t &y) const;
	bool ScreenCoordToGridPosition(const int32_t x, const int32_t y, uint32_t &xidx, uint32_t &yidx) const;

	void GridLine(const uint32_t xidx1, const uint32_t yidx1, const uint32_t xidx2, const uint32_t yidx2, const bool withpoints) const;
	void GridBox(const uint32_t xidx, const uint32_t yidx, const uint32_t size) const;
	void DrawButton(const uint8_t num, const char *text1, const char *text2, const bool selected) const;

	void DrawShip(const int32_t x, const int32_t y, const float scale, const float rotrad) const;

	bool ValidateShip(OutputStringStream &encoded, OutputStringStream &err) const;
	bool EncodeShip(OutputStringStream &encoded) const;

	uint64_t m_ticks;
	uint8_t m_mode;

	struct point
	{
		bool m_used;
		uint8_t m_xidx;
		uint8_t m_yidx;
		point ReflectX() const	{ point p=*this; p.m_xidx=(p.m_xidx<7 ? 7+(7-p.m_xidx) : 7-(p.m_xidx-7)); return p;};
	};

	int8_t m_selectedpoint;
	int8_t m_weaponpoint;
	point m_points[10];

};
