#include "binding.h"

#include <script/luah.h>
#include <script/lua_puper.h>

#include "tile_system.h"

using namespace oak::luah;


//tile getTile(chunk, x, y)
int c_getTile(lua_State *L) {
	Chunk& chunk = toValue<Chunk&>(L, 1);
	int x = toValue<int>(L, 2);
	int y = toValue<int>(L, 3);
	lua_settop(L, 0);

	auto &tile = chunk.getTile(x, y);

	lua_newtable(L);
	oak::LuaPuper puper{ L, 1 };
	pup(puper, tile, {});

	return 1;
}

//void setTile(chunk, x, y, tile)
int c_setTile(lua_State *L) {
	Chunk &chunk = toValue<Chunk&>(L, 1);
	int x = toValue<int>(L, 2);
	int y = toValue<int>(L, 3);

	auto &tile = chunk.getTile(x, y);

	oak::LuaPuper puper{ L, 4 };
	puper.setIo(oak::util::PuperIo::IN);
	pup(puper, tile, {});

	lua_settop(L, 0);

	return 0;
}

//chunk getChunk(tileSystem, x, y)
int c_getChunk(lua_State *L) {
	TileSystem& tileSystem = toValue<TileSystem&>(L, 1);
	int x = toValue<int>(L, 2);
	int y = toValue<int>(L, 3);
	lua_settop(L, 0);

	auto &chunk = tileSystem.getChunk(x, y);

	pushValue(L, chunk);

	return 1;
}

void initBindings(lua_State *L) {
	addFunctionToMetatable(L, "chunk", "getTile", c_getTile);
	addFunctionToMetatable(L, "chunk", "setTile", c_setTile);

	addFunctionToMetatable(L, "tile_system", "getChunk", c_getChunk);
}

namespace oak::luah {

	void pushValue(lua_State *L, TileSystem &tileSystem) {
		pushInstance(L, tileSystem);
		setMetatable(L, "tile_system");
	}

	template<> TileSystem& toValue(lua_State *L, int idx) {
		return toInstance<TileSystem>(L, idx);
	}

	void pushValue(lua_State *L, Chunk &chunk) {
		pushInstance(L, chunk);
		setMetatable(L, "chunk");
	}

	template<> Chunk& toValue(lua_State *L, int idx) {
		return toInstance<Chunk>(L, idx);
	}

}