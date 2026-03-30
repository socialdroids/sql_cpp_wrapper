#ifndef INCLUDE_INCLUDE_MAPSDB_HPP_
#define INCLUDE_INCLUDE_MAPSDB_HPP_

#include "DBAbstractionLayer.hpp"
#include <algorithm> // all_of
using namespace std;

class MapWaypointsDB : public DBInterface
{
public:
    typedef struct MapWaypoints
    {
        friend class MapWaypointsDB;
        int64_t map_id;
        string identifier;
        double x;
        double y;
        struct DBAngle
        {
            struct DBEuler
            {
                double roll, pitch, yaw;
            } euler;
            struct DBQuaternion
            {
                double x, y, z, w;
            } quaternion;
            bool is_quarternion = false;
            bool is_euler = true;
        } angle;

        friend std::ostream& operator<<(std::ostream& os, const MapWaypoints& v)
        {
            os << "MapWaypoints = {ID: " << v.id << "\n\tMap ID: " << v.map_id
               << "\n\tIdentifier: " << v.identifier << "\n\tPosition: (" << v.x
               << ", " << v.y << ", " << v.angle.euler.yaw
               << " rad)\n\tCreated At: " << asctime(&v.created_at)
               << "\tUpdated At: " << asctime(&v.updated_at) << "}\n";
            return os;
        }

        /**
         * @brief O ID é único e definido pelo próprio banco de dados, esse
         * método só pode ser utilizado quando o ID for conhecido através de
         * algum get, como: getByMap ou getByMapIdentifier.
         *
         * Ao adicionar uma entrada, o atributo `id` também é preenchido por
         * referência com o valor atribuído pelo banco de dados, permitindo que
         * o usuário saiba o ID para atribuir pontos.
         *
         * @param _id
         */
        void setID(int64_t _id)
        {
            id = _id;
        }

        int64_t getID() const
        {
            return id;
        }

    private:
        int64_t id;
        tm created_at;
        tm updated_at;
    } MapWaypoints;

    MapWaypointsDB(SQLManager* _manager, const string& _table)
        : DBInterface(_manager, _table)
    {
        // insert, update, primary
        using InsertBehavior = SQLSchema::InsertBehavior;
        using UpdateBehavior = SQLSchema::UpdateBehavior;

        sc.addColumn("id", SQLDataType::BigInt, InsertBehavior::kIgnore,
                     UpdateBehavior::kIgnore); // SERIAL
        sc.addColumn("map_id", SQLDataType::BigInt, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
        sc.addColumn("identifier", SQLDataType::Text, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
        sc.addColumn("x", SQLDataType::Double, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
        sc.addColumn("y", SQLDataType::Double, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
        sc.addColumn("yaw", SQLDataType::Double, InsertBehavior::kUse,
                     UpdateBehavior::kUse);

        sc.addColumn("created_at", SQLDataType::DateTime, InsertBehavior::kUse,
                     UpdateBehavior::kIgnore);
        sc.addColumn("updated_at", SQLDataType::DateTime, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
    }

    ~MapWaypointsDB()
    {
    }

    bool add(MapWaypoints& _waypoint)
    {
        time_t now = time(0);
        SQLEntry entry;
        entry.setValue("map_id", _waypoint.map_id);
        entry.setValue("identifier", _waypoint.identifier);
        entry.setValue("x", _waypoint.x);
        entry.setValue("y", _waypoint.y);

        double yaw = _waypoint.angle.euler.yaw;
        if (_waypoint.angle.is_quarternion)
        {
            throw invalid_argument(
                "Quaternion to Euler conversion not implemented!");
        }
        entry.setValue("yaw", yaw);

        entry.setValue("created_at", *localtime(&now));
        entry.setValue("updated_at", *localtime(&now));
        if (db->insert(&sc, &entry))
        {
            _waypoint.setID(
                db->lastStatement().sequence_last("map_waypoints_id_seq"));
            return true;
        }
        else
        {
            _waypoint.setID(-1);
        }

        return false;
    }

    bool update(MapWaypoints const& _waypoint)
    {
        time_t now = time(0);
        SQLEntry entry;
        entry.setValue("id", _waypoint.id);
        entry.setValue("map_id", _waypoint.map_id);
        entry.setValue("identifier", _waypoint.identifier);
        entry.setValue("x", _waypoint.x);
        entry.setValue("y", _waypoint.y);

        double yaw = _waypoint.angle.euler.yaw;
        if (_waypoint.angle.is_quarternion)
        {
            throw invalid_argument(
                "Quaternion to Euler conversion not implemented!");
        }
        entry.setValue("yaw", yaw);

        entry.setValue("created_at", *localtime(&now));
        entry.setValue("updated_at", *localtime(&now));

        SQLConditionMap conditions;
        conditions["id"] = _waypoint.id;
        return db->update(&sc, &entry, conditions);
    }

    bool remove(int64_t _map_id, int64_t _id)
    {
        SQLConditionMap conditions;
        conditions["map_id"] = _map_id;
        conditions["id"] = _id;
        return db->remove(&sc, conditions);
    }

    bool remove(int64_t _map_id, string const& _identifier)
    {
        SQLConditionMap conditions;
        conditions["map_id"] = _map_id;
        conditions["identifier"] = _identifier;
        return db->remove(&sc, conditions);
    }

    bool removeAll(int64_t _map_id)
    {
        SQLConditionMap conditions;
        conditions["map_id"] = _map_id;
        return db->remove(&sc, conditions);
    }

    vector<bool> saveWaypoints(vector<MapWaypoints>& _waypoints)
    {
        vector<bool> out;
        out.reserve(_waypoints.size());

        for (MapWaypoints& point : _waypoints)
        {
            if (add(point) == false)
            {
                out.push_back(update(point));
            }
            else
            {
                out.push_back(true);
            }
        }
        return out;
    }

    vector<MapWaypoints> getByMapIdentifier(int64_t _map_id,
                                            string const& _identifier)
    {
        vector<string> cols = sc.getColumnsNames();
        SQLConditionMap conditions;
        conditions["map_id"] = _map_id;
        if (_identifier != "")
        {
            conditions["identifier"] = _identifier;
        }

        vector<SQLEntry> results = db->get(&sc, cols, conditions);
        vector<MapWaypoints> out;
        out.reserve(results.size());
        MapWaypoints aux;

        for (SQLEntry const& data : results)
        {
            aux.id = get<int64_t>(data.getValue("id"));
            aux.map_id = get<int64_t>(data.getValue("map_id"));
            aux.identifier = get<string>(data.getValue("identifier"));
            aux.x = get<double>(data.getValue("x"));
            aux.y = get<double>(data.getValue("y"));
            aux.angle.euler.yaw = get<double>(data.getValue("yaw"));
            aux.angle.is_euler = true;
            aux.created_at = get<tm>(data.getValue("created_at"));
            aux.updated_at = get<tm>(data.getValue("updated_at"));

            out.push_back(aux);
        }
        return out;
    }

    vector<MapWaypoints> getByMap(int64_t _map_id)
    {
        return getByMapIdentifier(_map_id, "");
    }

private:
};

class MapPolygonPointsDB : public DBInterface
{
    friend class MapPolygonsDB;

public:
    typedef struct PolygonPoint
    {
        int64_t map_id;
        int64_t polygon_id;
        int32_t point_index;
        double x;
        double y;

        friend std::ostream& operator<<(std::ostream& os, const PolygonPoint& v)
        {
            os << "{x: " << v.x << ", y: " << v.y
               << ", PolyID: " << v.polygon_id << ", MapID: " << v.map_id
               << ", Index: " << v.point_index << "}";
            return os;
        }
    } PolygonPoint;

    MapPolygonPointsDB(SQLManager* _manager, const string& _table)
        : DBInterface(_manager, _table)
    {
        // insert, update, primary
        using InsertBehavior = SQLSchema::InsertBehavior;
        using UpdateBehavior = SQLSchema::UpdateBehavior;

        sc.addColumn("map_id", SQLDataType::BigInt, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
        sc.addColumn("polygon_id", SQLDataType::BigInt, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
        sc.addColumn("point_index", SQLDataType::Integer, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
        sc.addColumn("x", SQLDataType::Double, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
        sc.addColumn("y", SQLDataType::Double, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
    }

    ~MapPolygonPointsDB()
    {
    }

private:
    bool add(PolygonPoint const& _point)
    {
        SQLEntry entry;
        entry.setValue("map_id", _point.map_id);
        entry.setValue("polygon_id", _point.polygon_id);
        entry.setValue("point_index", _point.point_index);
        entry.setValue("x", _point.x);
        entry.setValue("y", _point.y);

        return db->insert(&sc, &entry);
    }

    bool update(PolygonPoint const& _point)
    {
        SQLEntry entry;
        entry.setValue("map_id", _point.map_id);
        entry.setValue("polygon_id", _point.polygon_id);
        entry.setValue("point_index", _point.point_index);
        entry.setValue("x", _point.x);
        entry.setValue("y", _point.y);

        SQLConditionMap conditions;
        conditions["map_id"] = _point.map_id;
        conditions["polygon_id"] = _point.polygon_id;
        conditions["point_index"] = _point.point_index;
        return db->update(&sc, &entry, conditions);
    }

    bool remove(int64_t _polygon_id, int32_t _point_index)
    {
        SQLConditionMap conditions;
        conditions["map_id"] = _polygon_id;
        conditions["polygon_id"] = _polygon_id;
        conditions["point_index"] = _point_index;
        return db->remove(&sc, conditions);
    }

    vector<bool> addPoints(vector<PolygonPoint> const& _points)
    {
        vector<bool> out;
        out.reserve(_points.size());

        for (PolygonPoint const& point : _points)
        {
            if (not add(point))
            {
                out.push_back(update(point));
            }
            else
            {
                out.push_back(true);
            }
        }
        return out;
    }

    vector<bool> updatePoints(vector<PolygonPoint> const& _points)
    {
        vector<bool> out;
        out.reserve(_points.size());

        for (PolygonPoint const& point : _points)
        {
            out.push_back(update(point));
        }
        return out;
    }

    vector<PolygonPoint> getByPolygon(int64_t _polygon_id)
    {
        vector<string> cols = sc.getColumnsNames();
        SQLConditionMap conditions;
        conditions["polygon_id"] = _polygon_id;

        vector<SQLEntry> results = db->get(&sc, cols, conditions);
        vector<PolygonPoint> out;
        out.reserve(results.size());
        PolygonPoint aux;

        for (SQLEntry const& data : results)
        {
            aux.map_id = get<int64_t>(data.getValue("map_id"));
            aux.polygon_id = get<int64_t>(data.getValue("polygon_id"));
            aux.point_index = get<int32_t>(data.getValue("point_index"));
            aux.x = get<double>(data.getValue("x"));
            aux.y = get<double>(data.getValue("y"));

            out.push_back(aux);
        }
        return out;
    }
};

class MapPolygonsDB : public DBInterface
{
public:
    typedef enum PolygonAllowedTypes
    {
        ROOM = 0,
        SPEED,
        RESTRICTED,
        BORDER,
        N_TYPES
    } PolygonAllowedTypes;

    static constexpr const char* PolygonTypenames[N_TYPES] = {
        "room", "speed", "restricted", "border"};

    typedef struct MapPolygon
    {
        friend class MapPolygonsDB;
        int64_t map_id;
        string identifier;
        vector<MapPolygonPointsDB::PolygonPoint> points;

        friend std::ostream& operator<<(std::ostream& os, const MapPolygon& v)
        {
            os << "MapPolygon = {ID: " << v.id << "\n\tMap ID: " << v.map_id
               << "\n\tType: " << v.polygon_type
               << "\n\tIdentifier: " << v.identifier << "\n\tPoints: [";

            for (MapPolygonPointsDB::PolygonPoint const& pt : v.points)
            {
                os << pt << "\n\t\t";
            }

            os << "]\n\tCreated At: " << asctime(&v.created_at)
               << "\tUpdated At: " << asctime(&v.updated_at) << "}\n";
            return os;
        }

        /**
         * @brief O ID é único e definido pelo próprio banco de dados, esse
         * método só pode ser utilizado quando o ID for conhecido.
         *
         * Ao adicionar uma entrada, o atributo `id` também é preenchido por
         * referência com o valor atribuído pelo banco de dados, permitindo que
         * o usuário saiba o ID para atribuir pontos.
         *
         * @param _id
         */
        void setID(int64_t _id)
        {
            id = _id;

            for (MapPolygonPointsDB::PolygonPoint& pt : this->points)
            {
                pt.polygon_id = this->id;
            }
        }

        int64_t getID() const
        {
            return id;
        }

        void setType(MapPolygonsDB::PolygonAllowedTypes _type)
        {
            polygon_type = MapPolygonsDB::PolygonTypenames[_type];
        }

        string type() const
        {
            return polygon_type;
        }

        void createPoint(double _x, double _y)
        {
            MapPolygonPointsDB::PolygonPoint point;
            point.map_id = this->map_id;
            point.polygon_id = this->id;
            point.point_index = points.size();
            point.x = _x;
            point.y = _y;

            points.push_back(point);
        }

        void resetPoints()
        {
            points.clear();
        }

    private:
        int64_t id = -1;
        string polygon_type;
        tm created_at;
        tm updated_at;
    } MapPolygon;

    MapPolygonsDB(SQLManager* _manager, const string& _table,
                  MapPolygonPointsDB* _pointsDBref)
        : DBInterface(_manager, _table), pointsDB(_pointsDBref)
    {
        // insert, update, primary
        using InsertBehavior = SQLSchema::InsertBehavior;
        using UpdateBehavior = SQLSchema::UpdateBehavior;
        sc.addColumn("id", SQLDataType::BigInt, InsertBehavior::kIgnore,
                     UpdateBehavior::kIgnore); // SERIAL
        sc.addColumn("map_id", SQLDataType::BigInt, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
        sc.addColumn("polygon_type", SQLDataType::Text, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
        sc.addColumn("identifier", SQLDataType::Text, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
        sc.addColumn("created_at", SQLDataType::TimestampTZ,
                     InsertBehavior::kUse, UpdateBehavior::kIgnore);
        sc.addColumn("updated_at", SQLDataType::TimestampTZ,
                     InsertBehavior::kUse, UpdateBehavior::kUse);
    }

    ~MapPolygonsDB()
    {
    }

    bool add(MapPolygon& _polygon)
    {
        time_t now = time(0);
        SQLEntry entry;
        entry.setValue("map_id", _polygon.map_id);
        entry.setValue("polygon_type", _polygon.polygon_type);
        if (_polygon.polygon_type == PolygonTypenames[BORDER])
        {
            sc.updateColumn("identifier", SQLSchema::InsertBehavior::kIgnore,
                            SQLSchema::UpdateBehavior::kIgnore);
        }
        else
        {
            entry.setValue("identifier", _polygon.identifier);
        }

        entry.setValue("created_at", *localtime(&now));
        entry.setValue("updated_at", *localtime(&now));

        bool ret = true;
        if (db->insert(&sc, &entry))
        {
            _polygon.setID(
                db->lastStatement().sequence_last("map_polygons_id_seq"));

            if (not _polygon.points.empty())
            {
                vector<bool> pointsOk = pointsDB->addPoints(_polygon.points);
                ret = all_of(pointsOk.begin(), pointsOk.end(),
                             [](bool val) { return val == true; });
            }
        }
        else
        {
            _polygon.setID(-1);
            ret = false;
        }
        sc.updateColumn("identifier", SQLSchema::InsertBehavior::kUse,
                        SQLSchema::UpdateBehavior::kUse);

        return ret;
    }

    /**
     * @brief Atualiza um polígono e seus respectivos pontos
     *
     * Necessita que o id e map_id sejam conhecidos.
     *
     * @param _polygon
     * @return true se conseguiu atualizar o polígono e os pontos, false se
     * algum falhou.
     */
    bool update(MapPolygon const& _polygon)
    {
        time_t now = time(0);
        SQLEntry entry;
        entry.setValue("map_id", _polygon.map_id);
        entry.setValue("polygon_type", _polygon.polygon_type);

        if (_polygon.polygon_type == PolygonTypenames[BORDER])
        {
            sc.updateColumn("identifier", SQLSchema::InsertBehavior::kIgnore,
                            SQLSchema::UpdateBehavior::kIgnore);
        }
        else
        {
            entry.setValue("identifier", _polygon.identifier);
        }

        entry.setValue("updated_at", *localtime(&now));

        SQLConditionMap conditions;
        conditions["id"] = _polygon.id;
        conditions["map_id"] = _polygon.map_id;

        bool ret = true;
        if (db->update(&sc, &entry, conditions))
        {
            if (not _polygon.points.empty())
            {
                vector<bool> pointsOk = pointsDB->addPoints(_polygon.points);
                ret = all_of(pointsOk.begin(), pointsOk.end(),
                             [](bool val) { return val == true; });
            }
        }
        else
        {
            ret = false;
        }
        sc.updateColumn("identifier", SQLSchema::InsertBehavior::kUse,
                        SQLSchema::UpdateBehavior::kUse);

        return ret;
    }

    bool remove(int64_t _id)
    {
        SQLConditionMap conditions;
        conditions["id"] = _id;
        return db->remove(&sc, conditions);
    }

    bool remove(int64_t _id, int64_t _map_id)
    {
        SQLConditionMap conditions;
        conditions["id"] = _id;
        conditions["map_id"] = _map_id;
        return db->remove(&sc, conditions);
    }

    bool remove(int64_t _id, string _identifier)
    {
        SQLConditionMap conditions;
        conditions["id"] = _id;
        conditions["identifier"] = _identifier;
        return db->remove(&sc, conditions);
    }

    bool removeAll(int64_t _map_id)
    {
        SQLConditionMap conditions;
        conditions["map_id"] = _map_id;
        return db->remove(&sc, conditions);
    }

    vector<MapPolygon> getByMapIdentifier(int64_t _map_id,
                                          string const& _identifier)
    {
        SQLConditionMap conditions;
        conditions["map_id"] = _map_id;
        if (_identifier != "")
        {
            conditions["identifier"] = _identifier;
        }

        return getBy(conditions);
    }

    vector<MapPolygon> getByMapType(int64_t _map_id, string const& _type)
    {
        SQLConditionMap conditions;
        conditions["map_id"] = _map_id;
        if (_type != "")
        {
            conditions["type"] = _type;
        }

        return getBy(conditions);
    }

    vector<MapPolygon> getByMap(int64_t _map_id)
    {
        SQLConditionMap conditions;
        conditions["map_id"] = _map_id;
        return getBy(conditions);
    }

private:
    MapPolygonPointsDB* pointsDB;

    inline vector<MapPolygonPointsDB::PolygonPoint> getPoints(
        int64_t _polygon_id)
    {
        return pointsDB->getByPolygon(_polygon_id);
    }

    vector<MapPolygon> getBy(const SQLConditionMap& _conditions)
    {
        vector<string> cols = sc.getColumnsNames();

        vector<SQLEntry> results = db->get(&sc, cols, _conditions);
        vector<MapPolygon> out;
        out.reserve(results.size());
        MapPolygon aux;

        for (SQLEntry const& data : results)
        {
            aux.id = get<int64_t>(data.getValue("id"));
            aux.map_id = get<int64_t>(data.getValue("map_id"));
            aux.polygon_type = get<string>(data.getValue("polygon_type"));
            try
            {
                aux.identifier = get<string>(data.getValue("identifier"));
            }
            catch (invalid_argument& e)
            {
                // identifier column can be null
                aux.identifier = "";
            }

            aux.created_at = get<tm>(data.getValue("created_at"));
            aux.updated_at = get<tm>(data.getValue("updated_at"));
            aux.points = getPoints(aux.id);

            out.push_back(aux);
        }
        return out;
    }
};

class MapsDB : public DBInterface
{
public:
    typedef struct MapData
    {
        friend class MapsDB;
        string pgm_path;
        string yaml_path;
        string stl_path;
        string obstacles_pgm_path;

        friend std::ostream& operator<<(std::ostream& os, const MapData& v)
        {
            os << "MapData = {ID: " << v.id << "\n\tMap PGM: " << v.pgm_path
               << "\n\tMap Yaml: " << v.yaml_path
               << "\n\tMap STL: " << v.stl_path
               << "\n\tObstacles PGM: " << v.obstacles_pgm_path
               << "\n\tCreated At: " << asctime(&v.created_at)
               << "\tUpdated At: " << asctime(&v.updated_at) << "}\n";
            return os;
        }

        int64_t getId() const
        {
            return id;
        }

    private:
        int64_t id;
        tm created_at;
        tm updated_at;
    } MapData;

    MapsDB(SQLManager* _manager, const string& _table)
        : DBInterface(_manager, _table)

    {
        // insert, update, primary
        using InsertBehavior = SQLSchema::InsertBehavior;
        using UpdateBehavior = SQLSchema::UpdateBehavior;

        sc.addColumn("id", SQLDataType::BigInt, InsertBehavior::kIgnore,
                     UpdateBehavior::kIgnore); // SERIAL
        sc.addColumn("pgm_path", SQLDataType::Text, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
        sc.addColumn("yaml_path", SQLDataType::Text, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
        sc.addColumn("stl_path", SQLDataType::Text, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
        sc.addColumn("obstacles_pgm_path", SQLDataType::Text,
                     InsertBehavior::kUse, UpdateBehavior::kUse);

        sc.addColumn("resolution", SQLDataType::Double, InsertBehavior::kIgnore,
                     UpdateBehavior::kIgnore);
        sc.addColumn("width", SQLDataType::Integer, InsertBehavior::kIgnore,
                     UpdateBehavior::kIgnore);
        sc.addColumn("height", SQLDataType::Integer, InsertBehavior::kIgnore,
                     UpdateBehavior::kIgnore);
        sc.addColumn("negate", SQLDataType::Boolean, InsertBehavior::kIgnore,
                     UpdateBehavior::kIgnore);
        sc.addColumn("occupied_thresh", SQLDataType::Double,
                     InsertBehavior::kIgnore, UpdateBehavior::kIgnore);
        sc.addColumn("free_thresh", SQLDataType::Double,
                     InsertBehavior::kIgnore, UpdateBehavior::kIgnore);
        sc.addColumn("origin_x", SQLDataType::Double, InsertBehavior::kIgnore,
                     UpdateBehavior::kIgnore);
        sc.addColumn("origin_y", SQLDataType::Double, InsertBehavior::kIgnore,
                     UpdateBehavior::kIgnore);
        sc.addColumn("origin_yaw", SQLDataType::Double, InsertBehavior::kIgnore,
                     UpdateBehavior::kIgnore);

        sc.addColumn("created_at", SQLDataType::DateTime, InsertBehavior::kUse,
                     UpdateBehavior::kIgnore);
        sc.addColumn("updated_at", SQLDataType::DateTime, InsertBehavior::kUse,
                     UpdateBehavior::kUse);
    }

    ~MapsDB()
    {
    }

    bool save(const MapData& _data, int64_t& _newMapID)
    {
        time_t now = time(0);
        SQLEntry entry;
        entry.setValue("pgm_path", _data.pgm_path);
        entry.setValue("yaml_path", _data.yaml_path);
        entry.setValue("stl_path", _data.stl_path);
        entry.setValue("obstacles_pgm_path", _data.obstacles_pgm_path);

        entry.setValue("created_at", *localtime(&now));
        entry.setValue("updated_at", *localtime(&now));

        bool ok = db->insert(&sc, &entry);
        if (ok)
        {
            _newMapID = db->lastStatement().sequence_last("maps_id_seq");
        }
        return ok;
    }

    bool update(int64_t _id, const MapData& _data)
    {
        time_t now = time(0);
        SQLEntry entry;
        entry.setValue("id", _id);
        entry.setValue("pgm_path", _data.pgm_path);
        entry.setValue("yaml_path", _data.yaml_path);
        entry.setValue("stl_path", _data.stl_path);
        entry.setValue("obstacles_pgm_path", _data.obstacles_pgm_path);

        entry.setValue("updated_at", *localtime(&now));

        SQLConditionMap conditions;
        conditions["id"] = _id;
        return db->update(&sc, &entry, conditions);
    }

    bool remove(int64_t _id)
    {
        SQLConditionMap conditions;
        conditions["id"] = _id;
        return db->remove(&sc, conditions);
    }

    MapData getMap(int64_t _id)
    {
        vector<string> cols = sc.getColumnsNames();
        SQLConditionMap conditions;
        conditions["id"] = _id;
        vector<SQLEntry> results = db->get(&sc, cols, conditions);
        MapData out;
        out.id = -1;

        if (!results.empty())
        {
            out.id = get<int64_t>(results.at(0).getValue("id"));
            out.pgm_path = get<string>(results.at(0).getValue("pgm_path"));
            out.yaml_path = get<string>(results.at(0).getValue("yaml_path"));
            out.stl_path = get<string>(results.at(0).getValue("stl_path"));
            out.obstacles_pgm_path =
                get<string>(results.at(0).getValue("obstacles_pgm_path"));
            out.created_at = get<tm>(results.at(0).getValue("created_at"));
            out.updated_at = get<tm>(results.at(0).getValue("updated_at"));
        }
        return out;
    }

    vector<MapData> getAll()
    {
        vector<SQLEntry> results = db->getAllEntries(&sc);
        vector<MapData> out;
        MapData aux;

        for (SQLEntry const& entry : results)
        {
            aux.id = get<int64_t>(entry.getValue("id"));
            aux.pgm_path = get<string>(entry.getValue("pgm_path"));
            aux.yaml_path = get<string>(entry.getValue("yaml_path"));
            aux.stl_path = get<string>(entry.getValue("stl_path"));
            aux.obstacles_pgm_path =
                get<string>(entry.getValue("obstacles_pgm_path"));
            aux.created_at = get<tm>(entry.getValue("created_at"));
            aux.updated_at = get<tm>(entry.getValue("updated_at"));

            out.push_back(aux);
        }
        return out;
    }

private:
};

#endif // INCLUDE_INCLUDE_MAPSDB_HPP_
