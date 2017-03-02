/*
    This file is part of Kismet

    Kismet is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kismet is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Kismet; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __ENTRYTRACKER_H__
#define __ENTRYTRACKER_H__

#include "config.h"

#include <stdio.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <map>

#include <pthread.h>

#include "globalregistry.h"
#include "trackedelement.h"
#include "kis_net_microhttpd.h"

// Allocate and track named fields and give each one a custom int
class EntryTracker : public Kis_Net_Httpd_Stream_Handler, public LifetimeGlobal {
public:
    static shared_ptr<EntryTracker> create_entrytracker(GlobalRegistry *in_globalreg) {
        shared_ptr<EntryTracker> mon(new EntryTracker(in_globalreg));
        in_globalreg->entrytracker = mon.get();
        in_globalreg->RegisterLifetimeGlobal(mon);
        in_globalreg->InsertGlobal("ENTRY_TRACKER", mon);
        return mon;
    }

private:
    EntryTracker(GlobalRegistry *in_globalreg);

public:
    virtual ~EntryTracker();

    // Reserve a field name.  Field names are plain strings, which can
    // be used to derive namespaces later.
    // Return: Registered field number, or negative on error (such as field exists with
    // conflicting type)
    int RegisterField(string in_name, TrackerType in_type, string in_desc);

    // Reserve a field name, and return an instance.  If the field ALREADY EXISTS, return
    // an instance.
    shared_ptr<TrackerElement> RegisterAndGetField(string in_name, 
            TrackerType in_type, string in_desc);

    // Reserve a field name, include a builder instance of the field instead of a 
    // fixed type.  Used for building complex types w/ storage backed by trackable 
    // elements.
    // Returns: Registered field number, or negative on error (such as field exists with a 
    // static type.  No checking can be performed between two fields generated by builder
    // instances.)
    int RegisterField(string in_name, shared_ptr<TrackerElement> in_builder, 
            string in_desc);

    // Reserve a field name, and return an instance.  If the field ALREADY EXISTS, return
    // an instance.
    shared_ptr<TrackerElement> RegisterAndGetField(string in_name, 
            shared_ptr<TrackerElement> in_builder, string in_desc);

    int GetFieldId(string in_name);
    string GetFieldName(int in_id);

    // Get a field instance
    // Return: NULL if unknown
    shared_ptr<TrackerElement> GetTrackedInstance(string in_name);
    shared_ptr<TrackerElement> GetTrackedInstance(int in_id);

    // Register a serializer for auto-serialization based on type
    void RegisterSerializer(string type, shared_ptr<TrackerElementSerializer> in_ser);
    void RemoveSerializer(string type);
    bool CanSerialize(string type);
    bool Serialize(string type, std::stringstream &stream, SharedTrackerElement elem,
            TrackerElementSerializer::rename_map *name_map = NULL);

    // HTTP api
    virtual bool Httpd_VerifyPath(const char *path, const char *method);

    virtual void Httpd_CreateStreamResponse(Kis_Net_Httpd *httpd,
            Kis_Net_Httpd_Connection *connection,
            const char *url, const char *method, const char *upload_data,
            size_t *upload_data_size, std::stringstream &stream);

protected:
    GlobalRegistry *globalreg;

    pthread_mutex_t entry_mutex;

    int next_field_num;

    struct reserved_field {
        // ID we assigned
        int field_id;

        // How we create it
        string field_name;
        TrackerType track_type;

        // Or a builder instance
        shared_ptr<TrackerElement> builder;

        // Might as well track this for auto-doc
        string field_description;
    };

    map<string, shared_ptr<reserved_field> > field_name_map;
    typedef map<string, shared_ptr<reserved_field> >::iterator name_itr;

    map<int, shared_ptr<reserved_field> > field_id_map;
    typedef map<int, shared_ptr<reserved_field> >::iterator id_itr;

    map<string, shared_ptr<TrackerElementSerializer> > serializer_map;
    typedef map<string, shared_ptr<TrackerElementSerializer> >::iterator serial_itr;

};

#endif
