# ZoneGraph Lane Profiles (AAA Traffic)

AAA Traffic expects two `ZoneLaneProfile` data assets at these plugin content paths:

- `/AAAtrafficSystem/ZoneProfiles/CityBLDUrbanTwoLane`
  - 2 lanes, each ~350cm (total ~700cm)
  - `LaneTagName` = `Vehicles`
- `/AAAtrafficSystem/ZoneProfiles/CityBLDFootpath`
  - 1 lane, ~200cm
  - `LaneTagName` = `FootTraffic`

These are referenced from `UTrafficRoadFamilySettings` (`FRoadFamilyDefinition.VehicleLaneProfile` / `FootpathLaneProfile`)
and are preloaded in `TrafficRuntime` module startup to reduce runtime hitching.

