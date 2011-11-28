Perugia makes it easy for arduinos to talk over swarm

This is arduino 1.0 compatible only

To use:
git clone this project into your sketchbook/libraries/ folder.  Make sure to restart arduino if it was running.  You should then see example code listed under File->Examples->perugia.

BEFORE UPLOADING make sure to create a resource and add it to a swarm.  Then fill in
the swarm_id, resource_id and participation_key variables.  Also make sure that the
networking variables (mac, ip, gw, subnet) are valid.  Finally, uncomment the server
variable for the swarm server you wish to use (ping the server to make sure that
the IP address is still valid!)
