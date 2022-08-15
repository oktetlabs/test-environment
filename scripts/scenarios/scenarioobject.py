#
# Copyright (C) 2016-2022 OKTET Labs Ltd. All rights reserved.

import yaml
import copy

def _list(cls):
    return lambda data: [cls(x) for x in data] if data else []

class ScenarioObject(object):
    def __init__(self, data, fields):
        self._fields = fields
        for (field, kwargs) in self._fields:
            self.__read_field(data, field, **kwargs)

    def __read_field(self, data, field, cls=str, optional=False, skip=False):
        if optional and field not in data:
            return

        assert field in data, (field, data)
        setattr(self, field, cls(data[field]))
        del data[field]

    def add_field(self, field, value, cls=str, optional=False, skip=False):
        assert field not in self._fields
        for (exist_field, _) in self._fields:
            assert field != exist_field, (field, exist_field)
        self._fields.append((field, {
            'cls':      cls,
            'optional': optional,
            'skip':     skip,
        }))
        setattr(self, field, cls(value))

    @staticmethod
    def represent(dumper, data, short):
        items = []
        for field, kwargs in data._fields:
            if kwargs.get('skip', False) and short:
                continue
            attr = getattr(data, field, None)
            if kwargs.get('optional', False) and not attr:
                continue
            items.append((
                dumper.represent_data(field),
                dumper.represent_data(attr)
            ))
        return yaml.nodes.MappingNode(
            yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, items)

    @staticmethod
    def short_represent(dumper, data):
        return data.represent(dumper, data, True)

    @staticmethod
    def full_represent(dumper, data):
        return data.represent(dumper, data, False)

class Value(ScenarioObject):
    def __init__(self, data):
        super().__init__(None, [])

        if type(data) in [int, str, type(None)]:
            self.value = data
        elif type(data) == dict and len(data.keys()) == 1:
            self.value, self.comment = list(data.items())[0]
            self.comment = self.comment.replace('\n', ' ').strip(' ')
        else:
            assert 0, data
        if type(self.value) == str:
            self.value = self.value.replace('\n', ' ').strip(' ')

    @staticmethod
    def represent(dumper, data, short):
        if not hasattr(data, 'comment'):
            return dumper.represent_data(data.value)
        else:
            return yaml.nodes.MappingNode(
                yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, [(
                dumper.represent_data(data.value),
                dumper.represent_data(data.comment)
            )])

class Param(ScenarioObject):
    def __init__(self, data):
        super().__init__(data, [
            ('param', {}),
            ('description', {}),
            ('type', {'optional': True}),
            ('values', {'cls': _list(Value), 'optional': True}),
        ])
        self.description = self.description.replace('\n', ' ').strip(' ')
        assert not data, data.keys()

class Step(ScenarioObject):
    def __init__(self, data):
        super().__init__(None, [])

        if type(data) == str:
            self.step = data
        elif type(data) == dict and len(data.keys()) == 1:
            self.step, data = list(data.items())[0]
            self.sub = _list(Step)(data)
        else:
            assert 0, data
        self.step = self.step.replace('\n', ' ').strip(' ')

    @staticmethod
    def represent(dumper, data, short):
        if not hasattr(data, 'sub'):
            return dumper.represent_str(data.step)
        else:
            return yaml.nodes.MappingNode(
                yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, [(
                dumper.represent_str(data.step),
                dumper.represent_list(data.sub)
            )])

class Test(ScenarioObject):
    def __init__(self, data):
        super().__init__(data, [
            ('test', {}),
            ('summary', {}),
            ('ref', {'optional': True}),
            ('objective', {'optional': True}),
            ('params', {'cls': _list(Param), 'optional': True, 'skip': True}),
            ('steps', {'cls': _list(Step), 'optional': True, 'skip': True}),
        ])
        if hasattr(self, 'objective'):
            self.objective = self.objective.replace('\n', ' ').strip(' ')
        assert not data, data.keys()

    @staticmethod
    def represent(dumper, data, short):
        node = ScenarioObject.represent(dumper, data, short)
#        print(node.start_mark)
#        node.end_mark = '\n'
        return node

class Group(ScenarioObject):
    def __init__(self, data):
        super().__init__(data, [
            ('group', {}),
            ('summary', {}),
            ('objective', {}),
            ('sub', {'cls': _list(Sub), 'optional': True}),
            ('groups', {'cls': _list(Group), 'optional': True}),
            ('tests', {'cls': _list(Test), 'optional': True}),
        ])
        assert not data, data.keys()

class Sub(ScenarioObject):
    def __init__(self, data):
        super().__init__(data, [
            ('group', {}),
            ('summary', {}),
            ('objective', {}),
            ('sub', {'cls': _list(Sub), 'optional': True}),
            ('groups', {'cls': _list(Group), 'optional': True}),
            ('tests', {'cls': _list(Test), 'optional': True}),
        ])
        assert not data, data.keys()

class Context(ScenarioObject):
    def __init__(self, data):
        super().__init__(data, [
            ('groups', {'cls': _list(Group)}),
        ])
        assert not data, data.keys()
