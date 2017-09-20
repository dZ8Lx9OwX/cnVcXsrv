# encoding=utf-8
# Copyright © 2017 Intel Corporation

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

"""Create enum to string functions for vulkan using vk.xml."""

from __future__ import print_function
import argparse
import os
import textwrap
import xml.etree.cElementTree as et

from mako.template import Template

COPYRIGHT = textwrap.dedent(u"""\
    * Copyright © 2017 Intel Corporation
    *
    * Permission is hereby granted, free of charge, to any person obtaining a copy
    * of this software and associated documentation files (the "Software"), to deal
    * in the Software without restriction, including without limitation the rights
    * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    * copies of the Software, and to permit persons to whom the Software is
    * furnished to do so, subject to the following conditions:
    *
    * The above copyright notice and this permission notice shall be included in
    * all copies or substantial portions of the Software.
    *
    * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    * SOFTWARE.""")

C_TEMPLATE = Template(textwrap.dedent(u"""\
    /* Autogenerated file -- do not edit
     * generated by ${file}
     *
     ${copyright}
     */

    #include <vulkan/vulkan.h>
    #include <vulkan/vk_android_native_buffer.h>
    #include "util/macros.h"
    #include "vk_enum_to_str.h"

    % for enum in enums:

        const char *
        vk_${enum.name[2:]}_to_str(${enum.name} input)
        {
            switch(input) {
            % for v in enum.values:
                % if v in FOREIGN_ENUM_VALUES:

                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wswitch"
                % endif
                case ${v}:
                    return "${v}";
                % if v in FOREIGN_ENUM_VALUES:
                #pragma GCC diagnostic pop

                % endif
            % endfor
            default:
                unreachable("Undefined enum value.");
            }
        }
    %endfor"""),
    output_encoding='utf-8')

H_TEMPLATE = Template(textwrap.dedent(u"""\
    /* Autogenerated file -- do not edit
     * generated by ${file}
     *
     ${copyright}
     */

    #ifndef MESA_VK_ENUM_TO_STR_H
    #define MESA_VK_ENUM_TO_STR_H

    #include <vulkan/vulkan.h>
    #include <vulkan/vk_android_native_buffer.h>

    % for enum in enums:
        const char * vk_${enum.name[2:]}_to_str(${enum.name} input);
    % endfor

    #endif"""),
    output_encoding='utf-8')

# These enums are defined outside their respective enum blocks, and thus cause
# -Wswitch warnings.
FOREIGN_ENUM_VALUES = [
    "VK_STRUCTURE_TYPE_NATIVE_BUFFER_ANDROID",
]


class EnumFactory(object):
    """Factory for creating enums."""

    def __init__(self, type_):
        self.registry = {}
        self.type = type_

    def __call__(self, name):
        try:
            return self.registry[name]
        except KeyError:
            n = self.registry[name] = self.type(name)
        return n


class VkEnum(object):
    """Simple struct-like class representing a single Vulkan Enum."""

    def __init__(self, name, values=None):
        self.name = name
        self.values = values or []


def parse_xml(efactory, filename):
    """Parse the XML file. Accumulate results into the efactory.

    This parser is a memory efficient iterative XML parser that returns a list
    of VkEnum objects.
    """

    with open(filename, 'rb') as f:
        context = iter(et.iterparse(f, events=('start', 'end')))

        # This gives the root element, since goal is to iterate over the
        # elements without building a tree, this allows the root to be cleared
        # (erase the elements) after the children have been processed.
        _, root = next(context)

        for event, elem in context:
            if event == 'end' and elem.tag == 'enums':
                type_ = elem.attrib.get('type')
                if type_ == 'enum':
                    enum = efactory(elem.attrib['name'])
                    enum.values.extend([e.attrib['name'] for e in elem
                                        if e.tag == 'enum'])
            elif event == 'end' and elem.tag == 'extension':
                if elem.attrib['supported'] != 'vulkan':
                    continue
                for e in elem.findall('.//enum[@extends][@offset]'):
                    enum = efactory(e.attrib['extends'])
                    enum.values.append(e.attrib['name'])

            root.clear()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--xml', required=True,
                        help='Vulkan API XML files',
                        action='append',
                        dest='xml_files')
    parser.add_argument('--outdir',
                        help='Directory to put the generated files in',
                        required=True)

    args = parser.parse_args()

    efactory = EnumFactory(VkEnum)
    for filename in args.xml_files:
        parse_xml(efactory, filename)

    for template, file_ in [(C_TEMPLATE, os.path.join(args.outdir, 'vk_enum_to_str.c')),
                            (H_TEMPLATE, os.path.join(args.outdir, 'vk_enum_to_str.h'))]:
        with open(file_, 'wb') as f:
            f.write(template.render(
                file=os.path.basename(__file__),
                enums=efactory.registry.values(),
                copyright=COPYRIGHT,
                FOREIGN_ENUM_VALUES=FOREIGN_ENUM_VALUES))


if __name__ == '__main__':
    main()
