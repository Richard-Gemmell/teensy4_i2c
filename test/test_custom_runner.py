import click

from platformio.public import UnityTestRunner


class CustomTestRunner(UnityTestRunner):

    def on_testing_line_output(self, line):
        if super().parse_test_case(line) is None:
            click.echo(line, nl=False)
        super().on_testing_line_output(line)