/*
   Copyright (C) 2001-2006, William Joseph.
   All Rights Reserved.

   This file is part of GtkRadiant.

   GtkRadiant is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GtkRadiant is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GtkRadiant; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "entity.h"

#include "ifilter.h"
#include "selectable.h"
#include "namespace.h"

#include "scenelib.h"
#include "entitylib.h"
#include "eclasslib.h"
#include "pivot.h"

#include "targetable.h"
#include "uniquenames.h"
#include "namekeys.h"
#include "stream/stringstream.h"
#include "filters.h"


#include "miscmodel.h"
#include "light.h"
#include "group.h"
#include "eclassmodel.h"
#include "generic.h"
#include "doom3group.h"


EGameType g_gameType;

inline scene::Node &entity_for_eclass(EntityClass *eclass)
{
    if ((string_compare_nocase_n(eclass->name(), "misc_", 5) == 0 &&
         string_equal_nocase(eclass->name() + string_length(eclass->name()) - 5,
                             "model")) // misc_*model (also misc_model) // TODO make classname_* wrapper functions for this
        || classname_equal(eclass->name(), "model_static")) {
        return New_MiscModel(eclass);
    } else if (classname_equal(eclass->name(), "light")
               || classname_equal(eclass->name(), "lightJunior")) {
        return New_Light(eclass);
    }
    if (!eclass->fixedsize) {
        if (g_gameType == eGameTypeDoom3) {
            return New_Doom3Group(eclass);
        } else {
            return New_Group(eclass);
        }
    } else if (!string_empty(eclass->modelpath())) {
        return New_EclassModel(eclass);
    } else {
        return New_GenericEntity(eclass);
    }
}

void Entity_setName(Entity &entity, const char *name)
{
    entity.setKeyValue("name", name);
}

typedef ReferenceCaller<Entity, void(const char *), Entity_setName> EntitySetNameCaller;

inline Namespaced *Node_getNamespaced(scene::Node &node)
{
    return NodeTypeCast<Namespaced>::cast(node);
}

inline scene::Node &node_for_eclass(EntityClass *eclass)
{
    scene::Node &node = entity_for_eclass(eclass);
    Node_getEntity(node)->setKeyValue("classname", eclass->name());

    if (g_gameType == eGameTypeDoom3
        && string_not_empty(eclass->name())
        && !string_equal(eclass->name(), "worldspawn")
        && !string_equal(eclass->name(), "UNKNOWN_CLASS")) {
        char buffer[1024];
        strcpy(buffer, eclass->name());
        strcat(buffer, "_1");
        GlobalNamespace().makeUnique(buffer, EntitySetNameCaller(*Node_getEntity(node)));
    }

    Namespaced *namespaced = Node_getNamespaced(node);
    if (namespaced != 0) {
        namespaced->setNamespace(GlobalNamespace());
    }

    return node;
}

EntityCreator::KeyValueChangedFunc EntityKeyValues::m_entityKeyValueChanged = 0;
EntityCreator::KeyValueChangedFunc KeyValue::m_entityKeyValueChanged = 0;
Counter *EntityKeyValues::m_counter = 0;

bool g_showNames = true;
bool g_showAngles = true;
bool g_newLightDraw = true;
bool g_lightRadii = false;

class ConnectEntities {
public:
    Entity *m_e1;
    Entity *m_e2;
    int m_index;

    ConnectEntities(Entity *e1, Entity *e2, int index) : m_e1(e1), m_e2(e2), m_index(index)
    {
    }

    const char *keyname()
    {
        StringOutputStream key(16);
        if (m_index <= 0) {
            return "target";
        }
        if (m_index == 1) {
            return "killtarget";
        }
        key << "target" << m_index;
        return key.c_str();
    }

    void connect(const char *name)
    {
        m_e1->setKeyValue(keyname(), name);
        m_e2->setKeyValue("targetname", name);
    }

    typedef MemberCaller<ConnectEntities, void(const char *), &ConnectEntities::connect> ConnectCaller;
};

inline Entity *ScenePath_getEntity(const scene::Path &path)
{
    Entity *entity = Node_getEntity(path.top());
    if (entity == 0) {
        entity = Node_getEntity(path.parent());
    }
    return entity;
}

class Quake3EntityCreator : public EntityCreator {
public:
    scene::Node &createEntity(EntityClass *eclass)
    {
        return node_for_eclass(eclass);
    }

    void setKeyValueChangedFunc(KeyValueChangedFunc func)
    {
        EntityKeyValues::setKeyValueChangedFunc(func);
    }

    void setCounter(Counter *counter)
    {
        EntityKeyValues::setCounter(counter);
    }

    void connectEntities(const scene::Path &path, const scene::Path &targetPath, int index)
    {
        Entity *e1 = ScenePath_getEntity(path);
        Entity *e2 = ScenePath_getEntity(targetPath);

        if (e1 == 0 || e2 == 0) {
            globalErrorStream() << "entityConnectSelected: both of the selected instances must be an entity\n";
            return;
        }

        if (e1 == e2) {
            globalErrorStream()
                    << "entityConnectSelected: the selected instances must not both be from the same entity\n";
            return;
        }


        UndoableCommand undo("entityConnectSelected");

        if (g_gameType == eGameTypeDoom3) {
            StringOutputStream key(16);
            if (index >= 0) {
                key << "target";
                if (index != 0) {
                    key << index;
                }
                e1->setKeyValue(key.c_str(), e2->getKeyValue("name"));
                key.clear();
            } else {
                for (unsigned int i = 0;; ++i) {
                    key << "target";
                    if (i != 0) {
                        key << i;
                    }
                    const char *value = e1->getKeyValue(key.c_str());
                    if (string_empty(value)) {
                        e1->setKeyValue(key.c_str(), e2->getKeyValue("name"));
                        break;
                    }
                    key.clear();
                }
            }
        } else {
            ConnectEntities connector(e1, e2, index);
            const char *value = e2->getKeyValue("targetname");
            if (!string_empty(value)) {
                connector.connect(value);
            } else {
                const char *type = e2->getKeyValue("classname");
                if (string_empty(type)) {
                    type = "t";
                }
                StringOutputStream key(64);
                key << type << "1";
                GlobalNamespace().makeUnique(key.c_str(), ConnectEntities::ConnectCaller(connector));
            }
        }

        SceneChangeNotify();
    }

    void setLightRadii(bool lightRadii)
    {
        g_lightRadii = lightRadii;
    }

    bool getLightRadii() const
    {
        return g_lightRadii;
    }

    void setShowNames(bool showNames)
    {
        g_showNames = showNames;
    }

    bool getShowNames()
    {
        return g_showNames;
    }

    void setShowAngles(bool showAngles)
    {
        g_showAngles = showAngles;
    }

    bool getShowAngles()
    {
        return g_showAngles;
    }

    void printStatistics() const
    {
        StringPool_analyse(EntityKeyValues::getPool());
    }
};

Quake3EntityCreator g_Quake3EntityCreator;

EntityCreator &GetEntityCreator()
{
    return g_Quake3EntityCreator;
}


class filter_entity_classname : public EntityFilter {
    const char *m_classname;
public:
    filter_entity_classname(const char *classname) : m_classname(classname)
    {
    }

    bool filter(const Entity &entity) const
    {
        return string_equal(entity.getKeyValue("classname"), m_classname);
    }
};

class filter_entity_classgroup : public EntityFilter {
    const char *m_classgroup;
    std::size_t m_length;
public:
    filter_entity_classgroup(const char *classgroup) : m_classgroup(classgroup), m_length(string_length(m_classgroup))
    {
    }

    bool filter(const Entity &entity) const
    {
        return string_equal_n(entity.getKeyValue("classname"), m_classgroup, m_length);
    }
};

filter_entity_classname g_filter_entity_world("worldspawn");
filter_entity_classname g_filter_entity_func_group("func_group");
filter_entity_classname g_filter_entity_light("light");
filter_entity_classname g_filter_entity_misc_model("misc_model");
filter_entity_classname g_filter_entity_misc_gamemodel("misc_gamemodel");
filter_entity_classgroup g_filter_entity_trigger("trigger_");
filter_entity_classgroup g_filter_entity_path("path_");

class filter_entity_doom3model : public EntityFilter {
public:
    bool filter(const Entity &entity) const
    {
        return string_equal(entity.getKeyValue("classname"), "func_static")
               && !string_equal(entity.getKeyValue("model"), entity.getKeyValue("name"));
    }
};

filter_entity_doom3model g_filter_entity_doom3model;


void Entity_InitFilters()
{
    add_entity_filter(g_filter_entity_world, EXCLUDE_WORLD);
    add_entity_filter(g_filter_entity_func_group, EXCLUDE_WORLD);
    add_entity_filter(g_filter_entity_world, EXCLUDE_ENT, true);
    add_entity_filter(g_filter_entity_trigger, EXCLUDE_TRIGGERS);
    add_entity_filter(g_filter_entity_misc_model, EXCLUDE_MODELS);
    add_entity_filter(g_filter_entity_misc_gamemodel, EXCLUDE_MODELS);
    add_entity_filter(g_filter_entity_doom3model, EXCLUDE_MODELS);
    add_entity_filter(g_filter_entity_light, EXCLUDE_LIGHTS);
    add_entity_filter(g_filter_entity_path, EXCLUDE_PATHS);
}


#include "preferencesystem.h"

void Entity_Construct(EGameType gameType)
{
    g_gameType = gameType;
    if (g_gameType == eGameTypeDoom3) {
        g_targetable_nameKey = "name";

        Static<KeyIsName>::instance().m_keyIsName = keyIsNameDoom3;
        Static<KeyIsName>::instance().m_nameKey = "name";
    } else {
        Static<KeyIsName>::instance().m_keyIsName = keyIsNameQuake3;
        Static<KeyIsName>::instance().m_nameKey = "targetname";
    }

    GlobalPreferenceSystem().registerPreference("SI_ShowNames", make_property_string(g_showNames));
    GlobalPreferenceSystem().registerPreference("SI_ShowAngles", make_property_string(g_showAngles));
    GlobalPreferenceSystem().registerPreference("NewLightStyle", make_property_string(g_newLightDraw));
    GlobalPreferenceSystem().registerPreference("LightRadiuses", make_property_string(g_lightRadii));

    Entity_InitFilters();
    LightType lightType = LIGHTTYPE_DEFAULT;
    if (g_gameType == eGameTypeRTCW) {
        lightType = LIGHTTYPE_RTCW;
    } else if (g_gameType == eGameTypeDoom3) {
        lightType = LIGHTTYPE_DOOM3;
    }
    Light_Construct(lightType);
    MiscModel_construct();
    Doom3Group_construct();

    RenderablePivot::StaticShader::instance() = GlobalShaderCache().capture("$PIVOT");

    GlobalShaderCache().attachRenderable(StaticRenderableConnectionLines::instance());
}

void Entity_Destroy()
{
    GlobalShaderCache().detachRenderable(StaticRenderableConnectionLines::instance());

    GlobalShaderCache().release("$PIVOT");

    Doom3Group_destroy();
    MiscModel_destroy();
    Light_Destroy();
}
