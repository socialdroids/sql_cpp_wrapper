# SQL C++ Wrapper

Header only C++ SQL Wrapper for Postgres database.

Usage examples can be found in `main.cpp`.

## Dependencies

```shell
sudo apt install libcppdb-dev libcppdb-postgresql0
```

## Build

```shell
mkdir build
cd build
cmake ..
make -j
```

## References:
- [CppDB](https://cppcms.com/sql/cppdb/index.html) 
- [Postgres Backend](https://salsa.debian.org/debian/cppdb/-/blob/master/drivers/postgres_backend.cpp) 

### Database used for development:

```mermaid
erDiagram

	wifi_profiles {
		TEXT ssid
		TEXT password
		TIMESTAMPTZ created_at
		TIMESTAMPTZ updated_at
	}

	hotspot_settings {
		SMALLINT id
		TEXT ssid
		TEXT password
		TIMESTAMPTZ created_at
		TIMESTAMPTZ updated_at
	}

	robot_time_settings {
		SMALLINT id
		TIMESTAMPTZ configured_datetime
		BOOLEAN configured_datetime_use_utc
		BOOLEAN automatic_time_sync_enabled
		TIMESTAMPTZ created_at
		TIMESTAMPTZ updated_at
	}

	robot_working_allowed_intervals {
		BIGSERIAL id
		TEXT weekday
		TIME start_time
		TIME end_time
		TIMESTAMPTZ created_at
		TIMESTAMPTZ updated_at
	}

	maps {
		BIGSERIAL id
		TEXT pgm_path
		TEXT yaml_path
		TEXT stl_path
		TEXT obstacles_pgm_path
		DOUBLE  resolution
		INTEGER width
		INTEGER height
		BOOLEAN negate
		DOUBLE  occupied_thresh
		DOUBLE  free_thresh
		DOUBLE  origin_x
		DOUBLE  origin_y
		DOUBLE  origin_yaw
		TIMESTAMPTZ created_at
		TIMESTAMPTZ updated_at
	}

	map_waypoints {
		BIGSERIAL id
		BIGINT map_id
		TEXT identifier
		DOUBLE x
		DOUBLE y
		DOUBLE yaw
		TIMESTAMPTZ created_at
		TIMESTAMPTZ updated_at
	}

	map_polygons {
		BIGSERIAL id
		BIGINT map_id
		TEXT polygon_type
		TEXT identifier
		TIMESTAMPTZ created_at
		TIMESTAMPTZ updated_at
	}

	map_polygon_points {
    BIGINT polygon_id INTEGER point_index DOUBLE x DOUBLE y
	}
```
