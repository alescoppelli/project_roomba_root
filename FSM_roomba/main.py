# main.py -- put your code here!

import roomba

if __name__ == "__main__":

	robot = roomba.roomba("name","model")
	robot.add_state("ASLEEP")
        robot.add_state("OFF")
	robot.add_state("PASSIVE")
        robot.add_state("SAFE")
	robot.add_state("FULL")
	robot.add_event("WAKE_UP","ASLEEP","OFF")
	robot.add_event("START","OFF","PASSIVE")
	robot.add_event("STOP","PASSIVE","OFF")
	robot.add_event("SENSORS","PASSIVE","PASSIVE")
	robot.add_event("CONTROL","PASSIVE","SAFE")
	robot.add_event("DRIVE","SAFE","SAFE")
	robot.add_event("SENSORS","SAFE","SAFE")
	robot.add_event("SAFE","FULL","SAFE")
	robot.add_event("FULL","SAFE","FULL")
	robot.add_event("SENSORS","FULL","FULL")
	robot.add_event("DRIVE","FULL","FULL")
	robot.add_event("POWER","SAFE","ASLEEP")
	robot.add_event("POWER","FULL","ASLEEP")

	robot.set_initial_current_state("ASLEEP")






