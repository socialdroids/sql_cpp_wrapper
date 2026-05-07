#include <sql_cpp_wrapper/wrapper.h>

#include <chrono>
#include <thread>

void testWiFi(SQLManager* _manager)
{
    WiFiDB wifiDB(_manager, "wifi_profiles");
    std::cout << "[WiFi] Saving Config...\n";
    if (wifiDB.saveConfig("SOCIADLROIDS2", "Slinky2027"))
    {
        std::cout << "[WiFi] Config saved!\n";
    }
    else
    {
        std::cout << "[WiFi] Failed to save config, probably duplicate\n";
    }

    std::cout << "[WiFi] Showing all entries in DB!\n";
    for (const auto& config : wifiDB.getWiFiEntries())
    {
        std::cout << config;
    }

    std::cout << "[WiFi] Removing Config...\n";
    if (wifiDB.removeConfig("SOCIADLROIDS2", "Slinky2027"))
    {
        std::cout << "[WiFi] Config removed!\n";
    }
    else
    {
        std::cout << "[WiFi] Failed to remove config, probably doesn't exist\n";
    }

    std::cout << "[WiFi] Showing all entries in DB!\n";
    for (const auto& config : wifiDB.getWiFiEntries())
    {
        std::cout << config;
    }
    std::cout << "---------------------\n\n\n";
}

void testHotspot(SQLManager* _manager)
{
    HotspotSettingsDB hotspotDB(_manager, "hotspot_settings");
    std::cout << "[Hotspot] Saving Hotspot settings...\n";
    if (hotspotDB.save(1, "RobotHS-2", "SD2026"))
    {
        std::cout << "[Hotspot] Hotspot settings saved!\n";
    }
    else
    {
        std::cout << "[Hotspot] Failed to save hotspot settings, probably "
                     "duplicate\n";
    }
    std::cout << "[Hotspot] Show current settings\n";
    HotspotSettingsDB::HotspotSettings ret = hotspotDB.getSettings(1);
    if (ret.id != -1)
        std::cout << ret;

    std::cout << "[Hotspot] Remove current settings\n";
    if (hotspotDB.remove(1))
    {
        std::cout << "[Hotspot] Hotspot settings removed!\n";
    }
    else
    {
        std::cout << "[Hotspot] Failed to remove hotspot settings, probably "
                     "doesn't exist\n";
    }

    std::cout << "[Hotspot] Showing all entries in DB!\n";
    for (auto& n : hotspotDB.getHotspotEntries())
    {
        std::cout << n;
    }
    std::cout << "---------------------\n\n\n";
}

void testWorkInterval(SQLManager* _manager)
{
    RobotWorkIntervalsDB workIntervalDB(_manager,
                                        "robot_working_allowed_intervals");
    std::cout << "[Work Intervals] Add robot working interval...\n";
    if (workIntervalDB.add(RobotWorkIntervalsDB::TUESDAY, "06:00:00",
                           "23:00:00"))
    {
        std::cout << "[Work Intervals] Working interval saved!\n";
    }
    else
    {
        std::cout
            << "[Work Intervals] Failed to save working interval, probably "
               "duplicate\n";
    }

    std::cout << "[Work Intervals] Show all entries in DB!\n";
    for (auto& n : workIntervalDB.getIntervals(RobotWorkIntervalsDB::TUESDAY))
    {
        std::cout << n;
    }
    std::cout << "[Work Intervals] Clear interval for selected day...\n";
    if (workIntervalDB.clear(RobotWorkIntervalsDB::TUESDAY))
    {
        std::cout << "[Work Intervals] Clear interval successful!\n";
    }
    else
    {
        std::cout << "[Work Intervals] Failed to clear interval!\n";
    }

    std::cout << "[Work Intervals] Show all entries in DB!\n";
    for (auto& n : workIntervalDB.getIntervals(RobotWorkIntervalsDB::TUESDAY))
    {
        std::cout << n;
    }
    std::cout << "---------------------\n\n\n";
}

void testRobotTime(SQLManager* _manager)
{
    RobotTimeDB timeDB(_manager, "robot_time_settings");
    RobotTimeDB::TimeSettings settings, ret;
    settings.id = 1;

    std::time_t now = std::time(0);
    settings.configured_datetime = *std::localtime(&now);
    settings.configured_datetime_use_utc = false;
    settings.automatic_time_sync_enabled = false;

    std::cout << "[Time] Add/update time settings...\n";
    if (timeDB.save(settings))
    {
        std::cout << "[Time] Time added!\n";
    }
    else
    {
        std::cout << "[Time] Failed to add time, probably duplicate\n";
    }

    std::cout << "[Time] Get current time settings...\n";
    ret = timeDB.getSettings(1);
    if (ret.id != -1)
    {
        std::cout << ret;
    }
    else
    {
        std::cout << "[Time] Time settings not found!\n";
    }

    std::cout << "[Time] Remove current time settings...\n";
    if (timeDB.remove(1))
    {
        std::cout << "[Time] Time settings removed!\n";
    }
    else
    {
        std::cout << "[Time] Failed to remove current time settings!\n";
    }

    std::cout << "[Time] Showing all entries in DB!\n";
    for (auto& n : timeDB.getSettings())
    {
        std::cout << n;
    }

    std::cout << "---------------------\n\n\n";
}

void testMaps(SQLManager* _manager)
{
    MapsDB mapsDB(_manager, "maps");
    MapsDB::MapData mData;
    mData.pgm_path = "/home/socialdroids/map.pgm";
    mData.yaml_path = "/home/socialdroids/map.yaml";
    mData.stl_path = "/home/socialdroids/map_3d.stl";
    mData.obstacles_pgm_path = "/home/socialdroids/obstacles.pgm";

    int64_t mapID = -1;
    std::cout << "[Maps] Saving map data...\n";
    if (mapsDB.save(mData, mapID))
    {
        std::cout << "[Maps] Map data saved as Map ID " << mapID << "!\n";
    }
    else
    {
        std::cout << "[Maps] Failed to save map data!\n";
    }

    std::cout << "[Maps] Updating map yaml path\n";
    mData.yaml_path = "/home/socialdroids/map_2.yaml";
    if (mapsDB.update(mapID, mData))
    {
        std::cout << "[Maps] Map data updated\n";
    }
    else
    {
        std::cout
            << "[Maps] Failed to update map data, probably using wrong ID\n";
    }

    std::cout << "[Maps] Get map by ID\n";
    mData = mapsDB.getMap(mapID);
    if (mData.getId() != -1)
    {
        std::cout << mData;
    }
    else
    {
        std::cout << "[Maps] Failed to get map by ID\n";
    }

    std::cout << "[Maps] Showing all entries in DB!\n";
    for (auto const& n : mapsDB.getAll())
    {
        std::cout << n;
    }

    std::cout << "[Maps] Removing map with Map ID " << mapID << "...\n";
    if (mapsDB.remove(mapID))
    {
        std::cout << "[Maps] Removed!\n";
    }
    else
    {
        std::cout << "[Maps] Failed to remove map!\n";
    }

    std::cout << "---------------------\n\n\n";
}

void testWaypoints(SQLManager* _manager)
{
    MapWaypointsDB waypointsDB(_manager, "map_waypoints");
    MapWaypointsDB::MapWaypoints wp;
    int64_t mapID = 1;
    wp.map_id = mapID;

    wp.identifier = "João José 2";
    wp.x = 1.5;
    wp.y = 0.73;
    wp.angle.euler.yaw = 0.75;
    std::cout << "[Waypoints] Add waypoint...\n";
    if (waypointsDB.add(wp))
    {
        std::cout << "[Waypoints] Waypoint added! ID = " << wp.getID() << "\n";
        wp.x = 10.5;
        std::cout << "[Waypoints] Update waypoint x to " << wp.x << "\n";
        if (waypointsDB.update(wp))
        {
            std::cout << "[Waypoints] Waypoint updated!\n";
        }
        else
        {
            std::cout << "[Waypoints] Failed to update waypoint\n";
        }
    }
    else
    {
        std::cout << "[Waypoints] Waypoint add failed! ID = " << wp.getID()
                  << "\n";
    }

    wp.identifier = "base";
    wp.x = 0.;
    wp.y = 0.5;
    std::cout << "[Waypoints] Add waypoint...\n";
    if (waypointsDB.add(wp))
    {
        std::cout << "[Waypoints] Waypoint added! ID = " << wp.getID() << "\n";
    }
    else
    {
        std::cout << "[Waypoints] Waypoint add failed! ID = " << wp.getID()
                  << "\n";
    }

    std::cout << "[Waypoints] Showing all entries in DB for MapID " << mapID
              << "!\n";
    for (auto& n : waypointsDB.getByMap(mapID))
    {
        std::cout << n;
    }

    std::cout << "[Waypoints] Remove all waypoints for Map ID " << mapID
              << " ...\n";
    if (waypointsDB.removeAll(mapID))
    {
        std::cout << "[Waypoints] Waypoints removed!\n";
    }
    else
    {
        std::cout << "[Waypoints] Failed to remove waypoints!\n";
    }

    std::cout << "[Waypoints] Showing all entries in DB for MapID " << mapID
              << "!\n";
    for (auto& n : waypointsDB.getByMap(mapID))
    {
        std::cout << n;
    }

    std::cout << "---------------------\n\n\n";
}

void testPolygons(SQLManager* _manager)
{
    MapPolygonPointsDB polyPointsDB(_manager, "map_polygon_points");
    MapPolygonsDB polygonsDB(_manager, "map_polygons", &polyPointsDB);
    MapPolygonsDB::MapPolygon poly;
    int64_t mapID = 1;

    poly.map_id = mapID;

    poly.setType(MapPolygonsDB::PolygonAllowedTypes::ROOM);
    poly.identifier = "Room 503";

    std::cout << "[Polygon] Create polygon with 4 points\n";
    poly.createPoint(10, 20);
    poly.createPoint(11, 21);
    poly.createPoint(12, 22);
    poly.createPoint(13, 23);

    std::cout << "[Polygon] Add polygon to map with MapID " << mapID << "\n";
    if (polygonsDB.add(poly))
    {
        std::cout << "[Polygon] Polygon added!\n";
    }
    else
    {
        std::cout << "[Polygon] Failed to add polygon!\n";
    }

    poly.setType(MapPolygonsDB::PolygonAllowedTypes::BORDER);
    poly.identifier = "";

    std::cout << "[Polygon] Create polygon with 4 points\n";
    poly.resetPoints();
    poly.createPoint(0, 0);
    poly.createPoint(1, 1);
    poly.createPoint(2, 2);
    poly.createPoint(3, 3);

    std::cout << "[Polygon] Add polygon to map with MapID " << mapID << "\n";
    if (polygonsDB.add(poly))
    {
        std::cout << "[Polygon] Polygon added!\n";
        std::cout << "[Polygon] Showing all entries in DB!\n";
        for (MapPolygonsDB::MapPolygon& n : polygonsDB.getByMap(mapID))
        {
            std::cout << n;
        }

        std::cout << "[Polygon] Reset polygon points!\n";
        poly.resetPoints();

        std::cout << "[Polygon] Create 5 points in polygon\n";
        poly.createPoint(5, 5);
        poly.createPoint(1, 1);
        poly.createPoint(2, 2);
        poly.createPoint(3, 3);
        poly.createPoint(25, 5 * poly.getID());

        std::cout << "[Polygon] Update polygon data\n";
        if (polygonsDB.update(poly))
        {
            std::cout << "[Polygon] Polygon updated!\n";
        }
        else
        {
            std::cout << "[Polygon] Failed to update polygon data!\n";
        }

        std::cout << "[Polygon] Show all polygons in map with MapID " << mapID
                  << "\n";
        for (MapPolygonsDB::MapPolygon& n : polygonsDB.getByMap(mapID))
        {
            std::cout << n;
        }

        std::cout << "[Polygon] Remove polygon with ID " << poly.getID()
                  << " from map with MapID " << mapID << "\n";
        if (polygonsDB.remove(poly.getID(), mapID))
        {
            std::cout << "[Polygon] Polygon removed!\n";
        }
        else
        {
            std::cout << "[Polygon] Failed to remove polygon!\n";
        }
    }
    else
    {
        std::cout << "[Polygon] Failed to add polygon!\n";
    }

    std::cout << "[Polygon] Showing all entries in DB!\n";
    for (MapPolygonsDB::MapPolygon& n : polygonsDB.getByMap(mapID))
    {
        std::cout << n;
    }

    std::cout << "[Polygon] Remove all polygons in map with MapID " << mapID
              << "\n";
    if (polygonsDB.removeAll(mapID))
    {
        std::cout << "[Polygon] Polygons removed!\n";
    }
    else
    {
        std::cout << "[Polygon] Failed to remove polygons!\n";
    }

    std::cout << "---------------------\n\n\n";
}

int main(int argc, char* argv[])
{
    SQLManager dbManager;
    bool ok = dbManager.init("setup_management", "postgres", "postgres");

    while (ok == false)
    {
        ok = dbManager.connect();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    while (1)
    {
        if (dbManager.isConnected())
        {
            testWiFi(&dbManager);
            std::this_thread::sleep_for(std::chrono::seconds(1));

            testHotspot(&dbManager);
            std::this_thread::sleep_for(std::chrono::seconds(1));

            testWorkInterval(&dbManager);
            std::this_thread::sleep_for(std::chrono::seconds(1));

            testRobotTime(&dbManager);
            std::this_thread::sleep_for(std::chrono::seconds(1));

            testMaps(&dbManager);
            std::this_thread::sleep_for(std::chrono::seconds(1));

            testWaypoints(&dbManager);
            std::this_thread::sleep_for(std::chrono::seconds(1));

            testPolygons(&dbManager);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        else {
            dbManager.connect();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}
