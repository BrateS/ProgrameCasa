#!/usr/bin/env python
import socket
import time
import sys
import logging
import pickle


DEFAULT_PORT = 3300
RELAY_ON = 'ON'
RELAY_OFF = 'OFF'
TIMEOUT_SECONDS = 2
CONFIG_FILENAME = "config_monitor.cfg"

def print_data_dict(data_dict):
    """
    Prints data_dict in pretty format.
    :param data_dict: dict parsed by extract_data
    :return: None
    """
    logging.debug(data_dict)
    try:
        room_index = list(data_dict['R'])[0]
        print('Room index: ' + str(room_index))
    except Exception as e:
        logging.exception(e)
        logging.warning("Could not get room index. Dict may be corrupt.")
        return
    for entry in list(data_dict['R'][room_index]):
        print("R" + str(entry) + " is " +
              str(int_to_relay(data_dict['R'][room_index][entry])), end=';')
    print()
    for entry in list(data_dict['T'][room_index]):
        print("T" + str(entry) + " is " +
              str(data_dict['T'][room_index][entry]), end='; ')
    print()


def relay_to_int(relay_mode):
    """
    Converts relay_mode to an int.
    :param relay_mode: string which is either 'ON' or 'OFF'
    :return: 1 ( 'ON' ) or 0
    """
    if relay_mode == RELAY_ON:
        return 1
    else:
        return 0


def int_to_relay(x):
    """
    The opposite of relay_to_int, converts to string.
    :param x: int noting the state of a relay
    :return: string which corresponds to the int
    """
    return RELAY_ON if x == 1 else RELAY_OFF


def read_line(s):
    """
    Receives messages until line end or until timeout.
    :param s: socket for the connection
    :return: string message
    """
    # Version copied from the internet CTRL-C + CTRL+V FTW
    # http://code.activestate.com/recipes/408859-socketrecv-three-ways-to-turn-it-into-recvall/
    total_data = []
    end = '\n'
    while True:
        data = s.recv(8192).decode()
        if end in data:
            total_data.append(data[:data.find(end)])
            break
        total_data.append(data)
        if len(total_data) > 1:
            # check if end_of_data was split
            last_pair = total_data[-2] + total_data[-1]
            if end in last_pair:
                total_data[-2] = last_pair[:last_pair.find(end)]
                total_data.pop()
                break
    return ''.join(total_data)


def relay_set(ip, port, id_releu, mode, retry=2):
    """
    Connect to a Arduino device and sets a relay to a mode.
    :param ip: ip of the arduino device
    :param port: port of the connection
    :param id_releu: id of the relay to be set
    :param mode: mode for the relay (on/off)
    :param retry: how many times to retry this operation until giving up
    :return: None
    """
    if not ('ON' in mode or "OFF" in mode):
        return
    message = "releu" + str(id_releu) + mode
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(TIMEOUT_SECONDS)
    s.connect((ip, port))
    while retry > 0:
        logging.debug('Sending message: ' + message)
        s.send((message + "\n").encode())
        try:
            data = read_line(s)
        except Exception as e:
            logging.exception(e)
            data = ''
        if message in str(data):  # need to make this more precise
            logging.debug('Message ack received.')
            return 0
        else:
            logging.warning('Failed ack. Retrying.' + 'Data: ' + str(data))
            time.sleep(2)
            retry -= 1
    s.send('close\n'.encode())
    return -1

def get_data(ip, port, retry=2):
    """
    Requests the data from a Arduino device
    :param ip: ip of the device
    :param port: port of the device for the connection
    :param retry: how many times to retry
    :return: string which contains the unparsed data
    """
    if retry > 0:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((ip, port))
            s.settimeout(TIMEOUT_SECONDS)
            message = "data"
            logging.debug('Sending message: ' + message)
            s.send((message + "\n").encode())
            data = read_line(s)
            s.send('close\n'.encode())
            if data == '':
                raise Exception("Data string is empty.")
            logging.debug(data)
            return data
        except Exception as e:
            logging.exception(e)
            logging.warning('Failed to connect: ' + ip + ' seems down.')
            get_data(ip, port, retry - 1)
    else:
        raise Exception('Could not get data.')


def extract_data(data_string):
    """
    Parses the data from the string from the Arduino device
    to a dictionary which can be further processed
    :param data_string: the string from arduino
    :return: the dictionary from the data string
    """
    ret = dict()
    try:
        entries = data_string.split('&&')
        for entry in entries:
            specifiers, value = entry.split('=')
            data_type, index_room, index = specifiers.split(':')
            index_room = int(index_room)
            index = int(index)
            if data_type not in ret:
                ret[data_type] = dict()
                ret[data_type][index_room] = dict()
            ret[data_type][index_room][index] = float(value.strip())
    except:
        logging.warning("Could not extract from data_string")
        return None
    return ret


def get_from(str_data, data_type='', index_room=-1, index=-1):
    """
    Goes through the dictionary data to get specific data.
    :param str_data: string from arduino
    :param data_type: type of the data (Temp or Relay)
    :param index_room: the index of the room
    :param index: index of the value in the room
    :return: value or dict of values
    """
    index_room = int(index_room)
    index = int(index)
    data_dict = extract_data(str_data)
    try:
        if data_type == '':
            return data_dict
        elif index_room == -1:
            return data_dict[data_type]
        elif index == -1:
            return data_dict[data_type][index_room]

        return data_dict[data_type][index_room][index]
    except:
        logging.warning("Could not extract from data_dict")
        return None


def turn_all(mode, ip):
    """
    Turn the relays of one device to a certain mode
    :param mode: mode to be set
    :param ip: ip of the device
    :return: 0 is success, other if not
    """
    data = get_data(ip, DEFAULT_PORT)
    data_dict = get_from(data)
    logging.debug(data_dict)
    try:
        room_index = list(data_dict['R'])[0]
        logging.debug('Room index: ' + str(room_index))
    except:
        logging.warning("Could not get room index. Dict may be corrupt.")
        return
    exceptions = 0
    for entry in list(data_dict['R'][room_index]):
        # Hardcoded case (Caz dubios cu zona de la cada)
        if '192.168.2.207' == ip and entry == 5:
            continue
        # Hardcoded case
        if int(data_dict['R'][room_index][entry]) != relay_to_int(mode):
            try:
                if relay_set(ip, DEFAULT_PORT, entry, mode):
                    raise Exception("Relay {} was not set.".format(entry))
            except Exception as e:
                logging.exception(e)
                exceptions += 1
    return exceptions


def test_connection(ip, port):
    """
    Opens a socket and connects to test a connection to a device.
    :param ip: ip of the device
    :param port: port to connect to
    :return: 0 if connection is successful
    """
    s = socket.socket()
    try:
        s.connect((ip, port))
    except Exception as e:
        logging.exception(e)
        return -3
    finally:
        s.close()
    return 0


class Room:
    """
    Class that holds info on the arduino device.
    """
    def __init__(self, name, ip, id):
        self.name = name
        self.ip = ip
        self.port = DEFAULT_PORT
        self.id = id

    def turn_all_on(self):
        return turn_all(RELAY_ON, self.ip)

    def turn_all_off(self):
        return turn_all(RELAY_OFF, self.ip)

    def is_down(self):
        if test_connection(self.ip, self.port):
            print(self.name + " is " + " down. Connection test failed.")
            return 1
        else:
            print(self.name + " is" + " up.")
            return 0

    def get_status(self):
        """
        Gets the data from the device
        :return: 0 if success, other if not
        """
        print('Status of ' + self.name)
        try:
            if self.is_down():
                return -3
            room_data = get_from(get_data(self.ip, self.port))
        except Exception as e:
            print("Data not available.")
            return -1
        print_data_dict(room_data)
        return 0

    def set_relay(self, index, mode):
        relay_set(self.ip, self.port, index, mode)


def error_handle():
    """
    Implements a mechanism to ask user if to retry.
    :return: 1 if user wants to retry, 0 if not
    """
    print("Error. Check log for errors. " + sys.argv[0].split('.')[0] + "_log.txt")
    user_in = input("Retry?(y/n)").strip()
    logging.debug("User input: " + user_in)
    if user_in == 'y':
        logging.debug("User retries.")
        return 1
    else:
        logging.debug("User chooses not to retry.")
        return 0

def restore_initial_config(rooms):
    config_file = None
    try:
        config_file = open(CONFIG_FILENAME, "rb")
    except Exception as exception:
        logging.warning("Failed to open config file.")
        logging.exception(exception)
        return 1

    data = pickle.load(config_file)
    for room in rooms:
        relays = data[room.id]
        for relay in relays.keys():
            room.set_relay(relay, int_to_relay(relays[relay]))
    if config_file:
        config_file.close()
    return 0

def save_running_config(rooms):
    try:
        config_file = open(CONFIG_FILENAME, "wb")
    except Exception as exception:
        logging.warning("Failed to open config file.")
        logging.exception(exception)
        return 3
    data = {}
    for room in rooms:
        if test_connection(room.ip, room.port):
            logging.debug("Room {} failed connection "
                          "test.".format(room.name))
            return 1
        try:
            room_data = get_from(get_data(room.ip, room.port),'R')
        except Exception as exception:
                logging.exception(exception)
                return 2
        data.update(room_data)
    if config_file:
        print(data)
        pickle.dump(data, config_file)
        config_file.close()
    return 0


logging.basicConfig(level=logging.DEBUG, filename=sys.argv[0].split('.')[0] + "_log.txt",
                    filemode='w', format='%(asctime)s %(levelname)s -'
                                         ' [%(filename)s:%(lineno)s - '
                                         '%(funcName)20s() ] - %(message)s')

Beci = Room('Beci', '192.168.2.204', id=2)
Panou_Hol = Room('Panou Hol', '192.168.2.206', id=3)
Panou_Scari = Room('Panou Scari', '192.168.2.207', id=4)
rooms = [Beci, Panou_Hol, Panou_Scari]

print("===HEATING SYSTEM MONITOR===")
while True:
    print("What would you like to manage today?")
    for room in rooms:
        room.is_down()
    choice = input("0)Get data of all rooms. 1)Beci 2)Panou_Hol 3)Panou_Scari\n"
                   "4)Restore from initial config\n"
                   "5)Save running config to initial config\n"
                   "6)Exit\n").strip()
    logging.debug("User choice: " + choice)
    choice = int(choice)
    if choice == 0:
        for room in rooms:
            if room.get_status():
                if error_handle():
                    room.get_status()
    elif 0 < choice < 4:
        room = rooms[choice - 1]
        print("Selected " + room.name)
        while True:
            print("What would you like to do with {}?".format(room.name))
            choice = input("0)Get status. \n1)Turn all relays on \n2)Turn all relays off \n"
                           "3)Turn specific relay on/off\n"
                           "4)Back to main menu\n").strip()
            logging.debug("User choice: " + choice)
            choice = int(choice)
            if choice == 0:
                if room.get_status():
                    if error_handle():
                        room.get_status()
            if choice == 1:
                if room.turn_all_on():
                    print("Some relays may have not been set.")
                    if error_handle():
                        if not room.turn_all_on():
                            print("Relays set.")
                        else:
                            print("Error. Check logs")
                else:
                    print("Everything on.")
            elif choice == 2:
                if room.turn_all_off():
                    if error_handle():
                        if not room.turn_all_off():
                            print("Relays set.")
                        else:
                            print("Error. Check logs")
                else:
                    print("Everything off.")
            elif choice == 3:
                try:
                    index = int(input("Which relay?(0,1,2...)\n"))
                    mode = int_to_relay(int(input("On or off?(1 - ON; 2 - OFF)\n")))
                    room.set_relay(index, mode)
                    print("Relay set.")
                except Exception as e:
                    logging.exception(e)
                    if error_handle():
                        index = int(input("Which relay?(0,1,2...)\n"))
                        mode = int_to_relay(int(input("On or off?(1 - ON; 2 - OFF\n")))
                        room.set_relay(index, mode)
                        print("Relay set.")
            elif choice == 4:
                print("Going back to menu.")
                break
    elif choice == 4:
        print("Initial config routine starting..")
        if restore_initial_config(rooms):
            print("Failed to restore initial config.")
        else:
            print("Initial config routine done.")
    elif choice == 5:
        if save_running_config(rooms):
            print("Failed to save running config.")
        print("Saving successful.")
    elif choice == 6:
        print("Exiting..")
        exit()

    else:
        print("Invalid choice. Exiting..")
        exit()
