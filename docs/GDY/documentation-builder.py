# iterate through the json schema and build documentation from it
import json
import os
from collections import defaultdict


class JsonSchemaToSphinx():

    def __init__(self):

        self._sphinx_string = ''
        self._id_to_nodes = {}
        self._refs_to_node = defaultdict(list)

        # Images for documentation
        self._sphinx_string += """
.. |red-check| image:: img/red-checkmark-16.png
"""

    def _get_or_default(self, node, key, default=None):
        return default if key not in node else node[key]

    def _generate_sphinx_node_example(self, node, node_name):
        node_example = ''

        if 'type' not in node:
            return ''

        node_type = node['type']
        example_top_level_nodes = []
        references = self._refs_to_node[node['$id']]
        if node_name is None and len(references) > 0:
            for (parent_node_name, parent_node) in references:
                example_top_level_nodes.append((parent_node_name, parent_node))
        else:
            example_top_level_nodes.append((node_name, node))

        node_example += f'''
:Example:
.. parsed-literal::

'''

        for (parent_node_name, parent_node) in example_top_level_nodes:
            if node_type == 'object':
                node_example += f'   {parent_node_name}:\n'
                for property_name, property_node in node['properties'].items():
                    id = property_node['$id']
                    ary = '- ' if parent_node['type'] == 'array' else ''
                    node_example += f'     {ary}:ref:`{property_name}<{id}>`: ... \n'

                node_example += '\n'

            else:
                return ''

        return node_example

    def _generate_sphinx_node_description(self, node, node_name):
        node_type = node['type'] if 'type' in node else 'any'
        node_description = ''
        #node_description += f':type: {node_type}\n'

        description_table_map = {}
        if node_type is not 'any':
            description_table_map['Data Type'] = node_type
        if node_name is not None:
            description_table_map['YAML Key'] = f'``{node_name}``'
        node_description += f':Description: {node["description"]}\n'
        if node_type == 'object':
            pass
        elif node_type == 'array':
            if 'maxItems' in node:
                description_table_map['Max Items'] = node['maxItems']
            if 'minItems' in node:
                description_table_map['Min Items'] = node['minItems']
        else:
            if 'oneOf' in node:
                one_of_table_data = []

                for option in node['oneOf']:
                    option_value = ''
                    if option['type'] == 'string':
                        if 'const' in option:
                            option_value = f'``{option["const"]}``'
                        elif 'pattern' in option:
                            option_value = f'``{option["pattern"]}``'
                    elif option['type'] == 'integer':
                        option_value = '``[integer]``'

                    one_of_table_data.append({'Value': option_value, 'Description': option['description']})

                node_description += f':Possible Values:\n' + self._generate_sphinx_list_table(['Value', 'Description'], one_of_table_data)
            if 'enum' in node:
                description_table_map['Allowed Values'] = f'``{"``, ``".join(node["enum"])}``'
            if 'const' in node:
                description_table_map['Allowed Values'] = f'``{node["const"]}``'
            if 'default' in node:
                description_table_map['Default Value'] = f'``{node["default"]}``'

        node_description += self._generate_sphinx_list_table(description_table_map.keys(), [description_table_map])

        return node_description

    def _generate_sphinx_list_table(self, headers, data_rows):

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

        sphinx_list_table += sphinx_headers + sphinx_values
        return sphinx_list_table

    def register_definitions(self, definition_node):
        if len(definition_node) > 0:
            for definition_name, new_definition in definition_node.items():
                self._id_to_nodes[new_definition['$id']] = new_definition

    def parse_definitions(self, definition_node):
        if len(definition_node) > 0:
            for definition_name, new_definition in definition_node.items():
                self.parse_node(new_definition)

    def parse_items(self, item_node, parent_node_name, parent_node):
        if len(item_node) > 0:
            if '$ref' in item_node:
                ref = item_node['$ref']
                self._refs_to_node[ref].append((parent_node_name, parent_node))
                referenced_node = self._id_to_nodes[ref]
                referenced_node_title = self._get_or_default(referenced_node, 'title', 'FIXME')
                referenced_node_id = item_node["$ref"]
                self._sphinx_string += f':Array Items:  :ref:`{referenced_node_title} <{referenced_node_id}>`\n\n'
            else:
                self.parse_node(item_node)

    def parse_properties(self, properties_node, required_properties=None):
        if len(properties_node) > 0:
            data_rows = []
            self._sphinx_string += ':Properties:\n'
            for property_name, new_property in properties_node.items():
                property_id = new_property['$id']
                is_required = required_properties is not None and (property_name in required_properties)
                required_img = '|red-check|' if is_required else ''
                data_rows.append({'Property': f':ref:`{property_name} <{property_id}>`', 'Required': required_img})

            self._sphinx_string += self._generate_sphinx_list_table(['Property', 'Required'], data_rows)

        for property_name, new_property in properties_node.items():
            self.parse_node(new_property, node_name=property_name)

    def parse_node(self, node, node_name=None):

        self._sphinx_string += '\n\n----------\n\n'
        self._sphinx_string += f'.. {node["$id"]}\n'

        self._id_to_nodes[node['$id']] = node

        # Create the title
        self._sphinx_string += f'.. _{node["$id"]}:\n\n'
        title = self._get_or_default(node, 'title', 'FIXME')
        self._sphinx_string += title + '\n'
        self._sphinx_string += '=' * len(title) + '\n\n'

        # Create the description
        self._sphinx_string += self._generate_sphinx_node_description(node, node_name)

        if 'definitions' in node:
            self.register_definitions(node['definitions'])

        self._sphinx_string += self._generate_sphinx_node_example(node, node_name)

        if 'items' in node:
            self.parse_items(node['items'], parent_node_name=node_name, parent_node=node)

        if 'properties' in node:
            required_properties = self._get_or_default(node, 'required', [])
            self.parse_properties(node['properties'], required_properties)

        if 'definitions' in node:
            self.parse_definitions(node['definitions'])

    def generate(self, node, node_name=None):
        """
        Handle generation of sphinx
        :param options:
        :return:
        """
        self.parse_node(node, node_name=node_name)

        return self._sphinx_string


if __name__ == '__main__':
    with open(os.path.realpath('../../resources/gdy-schema.json')) as schema_file:
        gdy_schema = json.load(schema_file)

    generator = JsonSchemaToSphinx()
    sphinx_string = generator.generate(gdy_schema["properties"]["Environment"], 'Environment')

    with open('sphinxdoc.rst', 'w') as f:
        f.write(sphinx_string)
