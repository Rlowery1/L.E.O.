# ZoneGraph Lane Profiles (AAA Traffic)

AAA Traffic expects two `TrafficZoneLaneProfile` data assets at these plugin content paths:

- `/AAAtrafficSystem/ZoneProfiles/CityBLDUrbanTwoLane`
  - CityBLD *vehicle* profiles treat `NumLanes` as **lanes per direction** and auto-generate opposing lanes in ZoneGraph.
    - Example: `NumLanes=1` -> 2 total lanes (1 forward + 1 backward), each ~350cm (total ~700cm)
  - `LaneTagName` = `Vehicles`
- `/AAAtrafficSystem/ZoneProfiles/CityBLDFootpath`
  - 1 lane, ~200cm
  - `LaneTagName` = `FootTraffic`

These are referenced from `UTrafficRoadFamilySettings` (`FRoadFamilyDefinition.VehicleLaneProfile` / `FootpathLaneProfile`)
and are preloaded in `TrafficRuntime` module startup to reduce runtime hitching.
