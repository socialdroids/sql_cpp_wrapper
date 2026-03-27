#ifndef INCLUDE_INCLUDE_NETWORKDB_HPP_
#define INCLUDE_INCLUDE_NETWORKDB_HPP_

#include "DBAbstractionLayer.hpp"
using namespace std;

class WiFiDB : public DBInterface
{
public:
    typedef struct WiFiConfig
    {
        string ssid;
        string password;
        tm created_at;
        tm updated_at;

        friend std::ostream& operator<<(std::ostream& os, const WiFiConfig& v)
        {
            os << "WiFiConfig = {SSID: " << v.ssid
               << ", Password: " << v.password
               << ", Created At: " << asctime(&v.created_at)
               << "\tUpdated At: " << asctime(&v.updated_at) << "}\n";
            return os;
        }
    } WiFiConfig;

    WiFiDB(SQLManager* _manager, const string& _table)
        : DBInterface(_manager, _table)
    {
        // insert, update, primary
        sc.addColumn("ssid", SQLDataType::String, true, true, true);
        sc.addColumn("password", SQLDataType::String, true, true, false);
        sc.addColumn("created_at", SQLDataType::DateTime, true, false, false);
        sc.addColumn("updated_at", SQLDataType::DateTime, true, true, false);
    }

    ~WiFiDB()
    {
    }

    bool saveConfig(string const& _ssid, string const& _pwd)
    {
        time_t now = time(0);
        SQLEntry entry;
        entry.setValue("ssid", _ssid);
        entry.setValue("password", _pwd);
        entry.setValue("created_at", *localtime(&now));
        entry.setValue("updated_at", *localtime(&now));

        if (db->insert(&sc, &entry) == false)
        {
            SQLConditionMap conditions;
            conditions["ssid"] = string(_ssid);
            return db->update(&sc, &entry, conditions);
        }
        return true;
    }

    bool removeConfig(string const& _ssid, string const& _pwd)
    {
        SQLConditionMap conditions;
        conditions["ssid"] = string(_ssid);
        if (_pwd != "")
        {
            conditions["password"] = string(_pwd);
        }
        return db->remove(&sc, conditions);
    }

    WiFiConfig getConfig(string const& _ssid)
    {
        vector<string> cols = sc.getColumnsNames();
        SQLConditionMap conditions;
        conditions["ssid"] = string(_ssid);
        vector<SQLEntry> results = db->get(&sc, cols, conditions);

        if (results.size() > 1)
        {
            cout << "More than one profile with the same SSID! Returning "
                    "first\n";
            return {get<string>(results.at(0).getValue("ssid")),
                    get<string>(results.at(0).getValue("password")),
                    get<tm>(results.at(0).getValue("created_at")),
                    get<tm>(results.at(0).getValue("updated_at"))};
        }
        return {.ssid = "", .password = ""};
    }

    vector<WiFiConfig> getWiFiEntries()
    {
        vector<WiFiConfig> entries;

        vector<SQLEntry> results = db->getAllEntries(&sc);
        for (const auto& entry : results)
        {
            entries.push_back({get<string>(entry.getValue("ssid")),
                               get<string>(entry.getValue("password")),
                               get<tm>(entry.getValue("created_at")),
                               get<tm>(entry.getValue("updated_at"))});
        }
        return entries;
    }

private:
};

class HotspotSettingsDB : public DBInterface
{
public:
    typedef struct HotspotSettings
    {
        int16_t id;
        string ssid;
        string password;
        tm created_at;
        tm updated_at;

        friend std::ostream& operator<<(std::ostream& os,
                                        const HotspotSettings& v)
        {
            os << "HotspotSettings = {ID: " << v.id << ", SSID: " << v.ssid
               << ", Password: " << v.password
               << ", Created At: " << asctime(&v.created_at)
               << "\tUpdated At: " << asctime(&v.updated_at) << "}\n";
            return os;
        }
    } HotspotSettings;

    HotspotSettingsDB(SQLManager* _manager, const string& _table)
        : DBInterface(_manager, _table)
    {
        // insert, update, primary
        sc.addColumn("id", SQLDataType::SmallInt, true, true, true);
        sc.addColumn("ssid", SQLDataType::Text, true, true, false);
        sc.addColumn("password", SQLDataType::Text, true, true, false);
        sc.addColumn("created_at", SQLDataType::TimestampTZ, true, false,
                     false);
        sc.addColumn("updated_at", SQLDataType::TimestampTZ, true, true, false);
    }

    ~HotspotSettingsDB()
    {
    }

    bool save(int16_t _id, string const& _ssid, string const& _pwd)
    {
        time_t now = time(0);
        SQLEntry entry;
        entry.setValue("id", _id);
        entry.setValue("ssid", _ssid);
        entry.setValue("password", _pwd);
        entry.setValue("created_at", *localtime(&now));
        entry.setValue("updated_at", *localtime(&now));

        if (db->insert(&sc, &entry) == false)
        {
            SQLConditionMap conditions;
            conditions["id"] = _id;
            return db->update(&sc, &entry, conditions);
        }
        return true;
    }

    bool remove(int16_t _id)
    {
        SQLConditionMap conditions;
        conditions["id"] = _id;
        return db->remove(&sc, conditions);
    }

    HotspotSettings getSettings(int16_t _id)
    {
        vector<string> cols = sc.getColumnsNames();
        SQLConditionMap conditions;
        conditions["id"] = _id;
        vector<SQLEntry> results = db->get(&sc, cols, conditions);
        HotspotSettings out;
        out.id = -1;

        if (!results.empty())
        {
            out.id = get<int16_t>(results.at(0).getValue("id"));
            out.ssid = get<string>(results.at(0).getValue("ssid"));
            out.password = get<string>(results.at(0).getValue("password"));
            out.created_at = get<tm>(results.at(0).getValue("created_at"));
            out.updated_at = get<tm>(results.at(0).getValue("updated_at"));
        }
        return out;
    }

    vector<HotspotSettings> getHotspotEntries()
    {
        vector<SQLEntry> results = db->getAllEntries(&sc);
        vector<HotspotSettings> entries;
        entries.reserve(results.size());

        for (const auto& entry : results)
        {
            entries.push_back({get<int16_t>(entry.getValue("id")),
                               get<string>(entry.getValue("ssid")),
                               get<string>(entry.getValue("password")),
                               get<tm>(entry.getValue("created_at")),
                               get<tm>(entry.getValue("updated_at"))});
        }
        return entries;
    }

private:
};

#endif // INCLUDE_INCLUDE_NETWORKDB_HPP_
