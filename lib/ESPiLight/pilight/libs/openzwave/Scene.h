//-----------------------------------------------------------------------------
//
//	Scene.h
//
//	A collection of ValueIDs to be used together.
//
//	Copyright (c) 2011 Greg Satz <satz@iranger.com>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#ifndef _Scene_H
#define _Scene_H

#include <string>
#include <vector>

#include "Defs.h"

class TiXmlElement;

namespace OpenZWave
{
	class ValueID;

	/** \brief Collection of ValueIDs to be treated as a unit.
	 */
	class Scene
	{
		friend class Manager;
		friend class Driver;
		friend class Node;

	//-----------------------------------------------------------------------------
	// Construction
	//-----------------------------------------------------------------------------
	private:
		Scene( uint8 const _sceneId );
		~Scene();

		static void WriteXML( string const& _name );
		static bool ReadScenes();

	//-----------------------------------------------------------------------------
	// Scene functions
	//-----------------------------------------------------------------------------
	private:
		static Scene* Get( uint8 const _sceneId );
		static uint8 GetAllScenes( uint8** _sceneIds );

		string const& GetLabel()const{ return m_label; }
		void SetLabel( string const &_label ){ m_label = _label; }

		bool AddValue( ValueID const& _valueId, string const& _value );
		bool RemoveValue( ValueID const& _valueId );
		void RemoveValues( uint32 const _homeId );
		static void RemoveValues( uint32 const _homeId, uint8 const _nodeId );
		int GetValues( vector<ValueID>* o_value );
		bool GetValue( ValueID const& _valueId, string* o_value );
		bool SetValue( ValueID const& _valueId, string const& _value );
		bool Activate();

	//-----------------------------------------------------------------------------
	// ValueID/value storage
	//-----------------------------------------------------------------------------
	private:
		class SceneStorage
		{
		public:
			SceneStorage( ValueID const& _id, string const& _value ): m_id( _id ), m_value( _value ) {};
			~SceneStorage() {};

			ValueID const m_id;
			string m_value;
		};
	//-----------------------------------------------------------------------------
	// Member variables
	//-----------------------------------------------------------------------------
	private:
		uint8					m_sceneId;
		string					m_label;
		vector<SceneStorage*>			m_values;
		static uint8				s_sceneCnt;
		static Scene*				s_scenes[256];
	};

} //namespace OpenZWave

#endif //_Scene_H
