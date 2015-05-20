#include "Userdata.h"

namespace LuaPoco
{

// LuaStateHolder implementation.
LuaStateHolder::LuaStateHolder(lua_State* L)
{
    state = L;
}

LuaStateHolder::~LuaStateHolder()
{
    if (state) lua_close(state);
}

lua_State* LuaStateHolder::extract()
{
    lua_State* L = state;
    state = NULL;
    return L;
}

// public member functions
Userdata::Userdata()
{
}

Userdata::~Userdata()
{
}

bool Userdata::copyToState(lua_State *L)
{
    return false;
}

// generic garbage collector destructor function which can be used by all classes
// that inherit from Userdata, provided they don't need to take special steps on GC.
int Userdata::metamethod__gc(lua_State* L)
{
    Userdata* ud = getPrivateUserdata(L, 1);
    if (ud) ud->~Userdata();
    else luaL_error(L, "attempting to __gc a NULL userdata!");
    return 0;
}

int pushPocoException(lua_State* L, const Poco::Exception& e)
{
    lua_pushnil(L);
    lua_pushlstring(L, e.displayText().c_str(), e.displayText().size());
    return 2;
}

int pushUnknownException(lua_State* L)
{
    lua_pushnil(L);
    lua_pushstring(L, "unknown Exception");
    return 2;
}

void setMetatableFunctions(lua_State* L, UserdataMethod* methods)
{
    while (methods->name && methods->fn)
    {
        lua_pushcfunction(L, methods->fn);
        lua_setfield(L, -2, methods->name);
        ++methods;
    }
}

void setupUserdataMetatable(lua_State* L, const char* metatableName, UserdataMethod* methods)
{
    luaL_newmetatable(L, metatableName);
    setMetatableFunctions(L, methods);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

void setupPocoUserdata(lua_State* L, Userdata* ud, const char* metatableName)
{
    luaL_getmetatable(L, metatableName);
    lua_setmetatable(L, -2);
    setPrivateUserdata(L, -1, ud);
}

void setupPrivateUserdata(lua_State* L)
{
    // start private table with 20 slots hash table slots for userdata.
    lua_createtable(L, 0, 20);
    // set weak key mode on table.
    lua_pushstring(L, "__mode");
    lua_pushstring(L, "k");
    lua_settable(L, -3);
    // set the table to be its own metatable
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);
    lua_setfield(L, LUA_REGISTRYINDEX, USERDATA_PRIVATE_TABLE);
}

// stores Userdata pointer in a private table with the derived userdata as a weak key.
void setPrivateUserdata(lua_State* L, int userdataIdx, Userdata* ud)
{
    // convert negative relative index to positive.
    userdataIdx = userdataIdx < 0 ? lua_gettop(L) + 1 + userdataIdx : userdataIdx;
    // retrieve the private userdata table.
    lua_getfield(L, LUA_REGISTRYINDEX, USERDATA_PRIVATE_TABLE);
    // private_table[derived_userdata] = base_light_userdata
    lua_pushvalue(L, userdataIdx);
    lua_pushlightuserdata(L, ud);
    lua_settable(L, -3);
    // remove private table from stack
    lua_pop(L, 1);
}
// retrieves the associated Userdata pointer for the derived pointer.
// (provides mechanism for type safe dynamic_cast from Userdata pointer back to the Derived pointer.)
Userdata* getPrivateUserdata(lua_State* L, int userdataIdx)
{
    Userdata* ud = NULL;
    userdataIdx = userdataIdx < 0 ? lua_gettop(L) + 1 + userdataIdx : userdataIdx;
    lua_getfield(L, LUA_REGISTRYINDEX, USERDATA_PRIVATE_TABLE);
    lua_pushvalue(L, userdataIdx);
    lua_gettable(L, -2);
    ud = static_cast<Userdata*>(lua_touserdata(L, -1));
    // pop lightuserdata and private table.
    lua_pop(L, 2);
    return ud;
}

} // LuaPoco
