import re
import sys

is_head = True

def translate_line(line):
    global is_head

    if '```' in line:
        if line.find('```') != 0:
            return line
        line = line.strip()

        if len(line) == 3:
            if is_head:
                ret =  '<pre><code class="">'
                is_head = False
                return ret
            else:
                ret = '</code></pre>'
                is_head = True
                return ret
        else:
            is_head = False
            return ('<pre><code class="%s">' % line[3:])
    return line


def insert_js(html_file_name, title):
    js_code = '''
<!DOCTYPE html><script src="https://cdn.jsdelivr.net/npm/texme@0.9.0"></script>
<head>
  <meta charset="UTF-8">
  <title>''' + title + '''</title>
  <link href="https://cdn.bootcss.com/highlight.js/9.12.0/styles/atom-one-dark.min.css" rel="stylesheet">
  <script src="https://cdn.bootcss.com/highlight.js/9.12.0/highlight.min.js"></script>
  <script >hljs.initHighlightingOnLoad();</script> 
</head>
    '''

    with open(html_file_name, "w") as f:
        f.write(js_code)
        f.write('\n\n')


def md2html(file_path):


    md_file = open(file_path, 'r')
    lines = md_file.readlines()
    md_file.close()

    new_liens = []
    for line in lines:
        line = translate_line(line)
        new_liens.append(line)


    html_file_name = file_path[:-3] + '.html'
    insert_js(html_file_name, file_path[:-3])

    html_file = open(html_file_name, 'a')
    html_file.writelines(new_liens)
    html_file.close()



if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("usage: python md2html.py xxx.md")
        exit(1)
    
    file_path = sys.argv[1]
    file_path = file_path.strip()
    if file_path[-3:] != '.md':
        print("usage: md2html.py xxx.md")
        exit(1)
    
    md2html(file_path)
    