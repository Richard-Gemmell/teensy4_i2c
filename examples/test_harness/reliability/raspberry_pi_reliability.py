from smbus2 import *


class Tester:
    address = 0x2D
    expected_from_teensy = "vast and cool and unsympathetic"
    message_bytes = list(bytes(expected_from_teensy, "UTF-8"))

    def __init__(self):
        self.bus: SMBus
        self.bytes_read = 0
        self.read_attempts = 0
        self.read_errors = 0
        self.bytes_written = 0
        self.write_attempts = 0
        self.write_errors = 0
        self.other_errors = 0

    def run(self):
        test_count = 0
        report_interval = 5000
        # test_size = 15_000_000
        # test_size = 1_000_000
        test_size = 100_000
        #test_size = 10000

        self.bus = SMBus(1)
        running = True
        try:
            while running and test_count < test_size:
                test_count += 1
                try:
                    self.read_and_check_message()
                    self.write_and_check_message()
                    if test_count % report_interval == 0:
                        self.report()
                except (KeyboardInterrupt, SystemExit):
                    running = False
                except:
                    self.other_errors += 1
                    # print("Error talking to slave.")
                    # The bus is probably stuck. The master needs to clear it but SMBUS doesn't
                    # seem to have a method for that. The spec says "the master should send nine
                    # clock pulses. One write should do the trick.
                    try:
                        self.bus.write_byte(0x00, 0x00)
                    except:
                        # Just swallow the exception
                        # print("Reset bus in case it's stuck.")
                        pass
        finally:
            self.bus.close()
            if test_count % report_interval != 0:
                self.report()

    def report(self):
        print('Read errors: {} read, {} other from {} read attempts. ({} bytes read)'.format(self.read_errors, self.other_errors, self.read_attempts, self.bytes_read))
        print('Write errors: {} write, {} other from {} write attempts. ({} bytes written)'.format(self.write_errors, self.other_errors, self.write_attempts, self.bytes_written))

    def read_and_check_message(self):
        self.read_attempts += 1
        self.bytes_written += 1
        # Read data and compare to expected value
        result = self.bus.read_i2c_block_data(self.address, 0, len(self.expected_from_teensy))
        self.bytes_read += len(result)
        text = bytes(result).decode('utf-8')
        if self.expected_from_teensy != text:
            self.read_errors += 1

    def write_and_check_message(self):
        self.write_attempts += 1
        self.bytes_written += len(self.message_bytes)+1
        # Read data and compare to expected value
        self.bus.write_i2c_block_data(self.address, 10, self.message_bytes)

        self.bytes_written += 1
        success = self.bus.read_i2c_block_data(self.address, 1, 1)
        self.bytes_read += 1
        if success[0] != 0xAA:
            self.write_errors += 1


if __name__ == '__main__':
    tester = Tester()
    tester.run()
