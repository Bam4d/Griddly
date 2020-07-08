# iterate through the json schema and build documentation from it
import json
import os
from pathlib import Path
from collections import defaultdict


class ObjectToSphinx():

    def __init__(self, node_name, node, id_to_nodes, refs_to_node, parent_node_path=None, is_array_item=False,
                 option=None):

        self._sphinx_string = ''
        self._id_to_nodes = id_to_nodes
        self._refs_to_node = refs_to_node

        self._node_name = node_name
        self._node = node

        self._is_array_item = is_array_item
        self._option = option

        if parent_node_path is not None:
            if self._option is not None:
                self._node_path = parent_node_path + '/oneOf/' + str(option)
            else:
                self._node_path = parent_node_path + '/' + node_name
        else:
            self._node_path = node_name

    def _get_or_default(self, node, key, default=None):
        return default if key not in node else node[key]

    def _generate_sphinx_node_description(self, node, node_name):
        node_type = node['type'] if 'type' in node else 'any'
        node_description = ''

        description_table_map = {}
        if node_type != 'any':
            description_table_map['Data Type'] = node_type
        if node_name is not None and not self._is_array_item:
            description_table_map['YAML Key'] = f'``{node_name}``'
        node_description += f':Description: {node["description"]}\n\n'

        if node_type == 'array':
            if 'maxItems' in node:
                description_table_map['Max Items'] = node['maxItems']
            if 'minItems' in node:
                description_table_map['Min Items'] = node['minItems']
        else:

            if 'minValue' in node:
                description_table_map['Min Value'] = f'``{node["minValue"]}``'
            if 'maxValue' in node:
                description_table_map['Max Value'] = f'``{node["maxValue"]}``'

            if 'oneOf' in node:
                one_of_table_data = []

                for option in node['oneOf']:
                    if '$ref' in option:
                        (option_name, option_path, option) = self._id_to_nodes[option['$ref']]

                    option_type = option['type']
                    option_value = ''
                    if option_type == 'string':
                        if 'const' in option:
                            option_value = f'``{option["const"]}``'
                        elif 'pattern' in option:
                            option_value = f'``{option["pattern"]}``'
                        else:
                            option_value = f'``[string]``'
                    elif option_type == 'integer':
                        option_value = '``[integer]``'
                    elif option_type == 'object':
                        option_node_title = option['title']
                        option_node_id = option['$id']
                        option_value = f':ref:`{option_node_title} <{option_node_id}>`'
                    elif option_type == 'array':
                        option_node_title = option['title']
                        if '$id' in option:
                            option_node_id = option['$id']
                            option_value = f':ref:`{option_node_title} <{option_node_id}>`'
                        else:
                            option_value = f'{option_node_title}'

                    one_of_table_data.append(
                        {'Value': option_value, 'Type': option_type, 'Description': option['description']})

                node_description += f':Possible Values:\n\n' + self._generate_sphinx_list_table(
                    ['Value', 'Type', 'Description'], one_of_table_data)
            if 'enum' in node:
                description_table_map['Allowed Values'] = f'``{"``, ``".join(node["enum"])}``'
            if 'const' in node:
                description_table_map['Allowed Values'] = f'``{node["const"]}``'
            if 'default' in node:
                description_table_map['Default Value'] = f'``{node["default"]}``'

        node_description += self._generate_sphinx_list_table(description_table_map.keys(), [description_table_map])

        return node_description

    def _generate_sphinx_list_table(self, headers, data_rows):

        if len(headers) == 0:
            return ''

        sphinx_list_table = '.. list-table::\n\n'

        sphinx_headers = ''
        for i, header in enumerate(headers):
            if i == 0:
                sphinx_headers += f'   * - **{header}**\n'
            else:
                sphinx_headers += f'     - **{header}**\n'

        sphinx_values = ''
        for row in data_rows:
            for i, header in enumerate(headers):
                if i == 0:
                    sphinx_values += f'   * - {self._get_or_default(row, header, "")}\n'
                else:
                    sphinx_values += f'     - {self._get_or_default(row, header, "")}\n'

        sphinx_list_table += sphinx_headers + sphinx_values + '\n\n'
        return sphinx_list_table

    def register_definitions(self, definition_node):
        if len(definition_node) > 0:
            for definition_name, new_definition in definition_node.items():
                definition_path = self._node_path + '/' + definition_name
                self._id_to_nodes[new_definition['$id']] = (definition_name, definition_path, new_definition)

    def parse_definitions(self, definition_node):
        if len(definition_node) > 0:
            for defined_node_name, defined_node in definition_node.items():
                generator = ObjectToSphinx(defined_node_name, defined_node, self._id_to_nodes, self._refs_to_node,
                                           parent_node_path=self._node_path, is_array_item=True)
                generator.generate()

    def parse_items(self, item_node, parent_node_name, parent_node_path, parent_node):

        array_type_table_data = []
        toc_references = ''
        if len(item_node) > 0:
            if '$ref' in item_node:
                ref = item_node['$ref']
                self._refs_to_node[ref].append((parent_node_name, parent_node_path, parent_node))
                (referenced_node_name, referenced_node_path, referenced_node) = self._id_to_nodes[ref]
                referenced_node_title = self._get_or_default(referenced_node, 'title', 'FIXME')
                referenced_node_description = self._get_or_default(referenced_node, 'description', '')
                referenced_node_id = item_node["$ref"]
                toc_references += f'   {referenced_node_path}/index\n'
                self._sphinx_string += f':Array Type:\n\n'

                array_type_table_data = [{
                    'Type': f' :ref:`{referenced_node_title}<{referenced_node_id}>` ',
                    'Description': referenced_node_description
                }]

            elif 'oneOf' in item_node:
                self._sphinx_string += f':Choose Between:\n\n'
                for option in item_node['oneOf']:
                    if '$ref' in option:
                        ref = option['$ref']
                        self._refs_to_node[ref].append((parent_node_name, parent_node_path, parent_node))
                        (referenced_node_name, referenced_node_path, referenced_node) = self._id_to_nodes[ref]
                        referenced_node_title = self._get_or_default(referenced_node, 'title', 'FIXME')
                        referenced_node_description = self._get_or_default(referenced_node, 'description', '')
                        referenced_node_id = option["$ref"]
                        toc_references += f'   {referenced_node_path}/index\n'
                        array_type_table_data.append({
                            'Type': f' :ref:`{referenced_node_title}<{referenced_node_id}>` ',
                            'Description': referenced_node_description
                        })


            elif 'anyOf' in item_node:
                self._sphinx_string += f':Array Types:\n\n'
                for option in item_node['anyOf']:
                    if '$ref' in option:
                        ref = option['$ref']
                        self._refs_to_node[ref].append((parent_node_name, parent_node_path, parent_node))
                        (referenced_node_name, referenced_node_path, referenced_node) = self._id_to_nodes[ref]
                        referenced_node_title = self._get_or_default(referenced_node, 'title', 'FIXME')
                        referenced_node_description = self._get_or_default(referenced_node, 'description', '')
                        referenced_node_id = option["$ref"]
                        toc_references += f'   {referenced_node_path}/index\n'
                        array_type_table_data.append({
                            'Type': f' :ref:`{referenced_node_title}<{referenced_node_id}>` ',
                            'Description': referenced_node_description
                        })

            else:
                generator = ObjectToSphinx('items', item_node, self._id_to_nodes, self._refs_to_node,
                                           parent_node_path=self._node_path, is_array_item=True)
                generator.generate()

                toc_references += f'   items/index\n'

                item_node_description = item_node['title']
                item_node_title = item_node['title']
                item_node_id = item_node['$id']

                self._sphinx_string += f':Array Type: \n\n'
                array_type_table_data = [{
                    'Type': f' :ref:`{item_node_title}<{item_node_id}>` ',
                    'Description': item_node_description
                }]

            self._sphinx_string += self._generate_sphinx_list_table(['Type', 'Description'],
                                                                    array_type_table_data)

            # generate toc tree for the array item
            self._sphinx_string += '.. toctree:: \n   :maxdepth: 5\n   :hidden:\n\n'
            self._sphinx_string += toc_references

    def parse_properties(self, properties_node, required_properties=None):

        if len(properties_node) > 0:
            data_rows = []
            self._sphinx_string += ':Properties:\n\n'
            for property_name, new_property in properties_node.items():
                property_id = ''
                if '$ref' in new_property:
                    property_id = new_property['$ref']
                else:
                    property_id = new_property['$id']

                is_required = required_properties is not None and (property_name in required_properties)
                required_img = '``true``' if is_required else ''
                data_rows.append({'Property': f' :ref:`{property_name} <{property_id}>` ', 'Required': required_img})

            self._sphinx_string += self._generate_sphinx_list_table(['Property', 'Required'], data_rows)

        for child_node_name, child_node in properties_node.items():
            if '$ref' not in child_node:
                generator = ObjectToSphinx(child_node_name, child_node, self._id_to_nodes, self._refs_to_node,
                                           parent_node_path=self._node_path)
                generator.generate()

        # generate toc tree for these properties
        self._sphinx_string += '.. toctree:: \n   :hidden:\n\n'
        for child_node_name, child_node in properties_node.items():
            if '$ref' in child_node:
                ref = child_node['$ref']
                (referenced_node_name, referenced_node_path, referenced_node) = self._id_to_nodes[ref]
                self._sphinx_string += f'   {referenced_node_path}/index\n'
            else:
                self._sphinx_string += f'   {child_node_name}/index\n'

    def generate(self):
        print(self._node_name)
        self._id_to_nodes[self._node['$id']] = (self._node_name, self._node_path, self._node)

        self._sphinx_string += f'.. _{self._node["$id"]}:\n\n'
        self._sphinx_string += f'.. {self._node["$id"]}\n\n'

        title = self._get_or_default(self._node, 'title', 'FIXME')
        self._sphinx_string += title + '\n'
        self._sphinx_string += '=' * len(title) + '\n\n'

        # Create the description
        self._sphinx_string += self._generate_sphinx_node_description(self._node, self._node_name)

        if 'definitions' in self._node:
            self.register_definitions(self._node['definitions'])

        if 'items' in self._node:
            self.parse_items(self._node['items'], parent_node_name=self._node_name, parent_node_path=self._node_path,
                             parent_node=self._node)

        if 'properties' in self._node:
            required_properties = self._get_or_default(self._node, 'required', [])
            self.parse_properties(self._node['properties'], required_properties)

        if 'definitions' in self._node:
            self.parse_definitions(self._node['definitions'])

        file_path = Path(f'../../../docs/{self._node_path}')
        file_path.mkdir(parents=True, exist_ok=True)
        with open(f'{file_path.resolve()}/index.rst', 'w') as f:
            f.write(self._sphinx_string)


def write_doc(schema, schema_title):
    generator = ObjectToSphinx(schema_title, schema, id_to_nodes={}, parent_node_path='/reference/GDY',
                               refs_to_node=defaultdict(list))
    generator.generate()


if __name__ == '__main__':
    with open(os.path.realpath('../../../resources/gdy-schema.json')) as schema_file:
        gdy_schema = json.load(schema_file)

    write_doc(gdy_schema['properties']['Environment'], 'Environment')
    write_doc(gdy_schema['properties']['Objects'], 'Objects')
    write_doc(gdy_schema['properties']['Actions'], 'Actions')
