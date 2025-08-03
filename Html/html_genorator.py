
def convertFile(fileName):
    with open(fileName + ".html") as input_file:
        input_file_lines = input_file.readlines()

    with open(fileName + ".txt", "w") as output_file:
        write_lines = []
        for html_line in input_file_lines:
            write_lines += ("client.println(\"" + html_line.replace("\"", "\\\"").rstrip() + "\");\n")
        output_file.writelines(write_lines)

convertFile("web_ui")
convertFile("finished_page")