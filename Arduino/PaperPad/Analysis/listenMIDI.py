import mido
import csv

def list_midi_ports():
    """Lists available MIDI input ports."""
    return mido.get_input_names()

def choose_midi_port():
    """Prompts the user to select a MIDI input port."""
    ports = list_midi_ports()
    if not ports:
        print("No MIDI input ports available.")
        return None

    print("Available MIDI input ports:")
    for i, port in enumerate(ports):
        print(f"{i}: {port}")

    while True:
        try:
            choice = int(input("Select the MIDI input port number: "))
            if 0 <= choice < len(ports):
                return ports[choice]
            else:
                print("Invalid selection. Please choose a valid port number.")
        except ValueError:
            print("Invalid input. Please enter a number.")

def combine_bytes(lsb, msb):
    """Combines the LSB and MSB into a single value."""
    return (msb << 7) | lsb

def listen_to_midi(port_name, csv_filename):
    """Listens to MIDI messages and writes them to a CSV file."""
    try:
        with mido.open_input(port_name) as inport, open(csv_filename, 'w', newline='') as csvfile:
            csv_writer = csv.writer(csvfile)
            csv_writer.writerow(['time', 'message_type', 'data_value', 'channel', 'combined_time_value'])
            
            print(f"Listening on {port_name}...")

            cumulative_time = 0

            try:
                while True:
                    # Receive the first message (data LSB)
                    msg_data_lsb = inport.receive()
                    if msg_data_lsb.type == 'control_change':
                        # Receive the second message (data MSB)
                        msg_data_msb = inport.receive()
                        if msg_data_msb.type == 'control_change':
                            data_value = combine_bytes(msg_data_lsb.value, msg_data_msb.value)

                            # Receive the third message (time LSB)
                            msg_time_lsb = inport.receive()
                            if msg_time_lsb.type == 'control_change':
                                # Receive the fourth message (time MSB)
                                msg_time_msb = inport.receive()
                                if msg_time_msb.type == 'control_change':
                                    combined_time_value = combine_bytes(msg_time_lsb.value, msg_time_msb.value)
                                    cumulative_time += combined_time_value / 1000000
                                    
                                    csv_writer.writerow([
                                        round(cumulative_time, 6), msg_data_lsb.type, data_value, msg_data_lsb.channel, combined_time_value
                                    ])
                                    print(f"Cumulative Time: {round(cumulative_time, 6)}, Data LSB: {msg_data_lsb.value}, Data MSB: {msg_data_msb.value}, Combined Data: {data_value}, Combined Time: {combined_time_value}")
            except KeyboardInterrupt:
                print("Stopping MIDI listener.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    midi_port_name = choose_midi_port()
    if midi_port_name:
        output_csv_filename = 'midi_output.csv'
        listen_to_midi(midi_port_name, output_csv_filename)
    else:
        print("No MIDI port selected. Exiting.")
