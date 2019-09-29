import abc
import enum
from lxml import etree


class TEXML(abc.ABC):
    class Status(enum.Enum):
        new = 0
        exist = 1

    @staticmethod
    @abc.abstractmethod
    def load(filename):
        """
        Load object from file

        :param filename: Filename use for load
        :return: Loaded object
        """
        pass

    @abc.abstractmethod
    def save(self, filename):
        """
        Save object to file

        :param filename: Filename use for save
        """
        pass

    @staticmethod
    def addnext_unique(root, node, condition):
        last_node = None
        for n in root.findall(node.tag):
            last_node = n
            if condition(n):
                return TEXML.Status.exist

        if last_node is None:
            root.insert(0, node)
        else:
            last_node.addnext(node)

        return TEXML.Status.new

    @staticmethod
    def node_equal(n1, n2):
        if n1.tag != n2.tag: return False
        if n1.text != n2.text: return False
        if n1.tail != n2.tail: return False
        if n1.attrib != n2.attrib: return False
        if len(n1) != len(n2): return False
        return all(TEXML.node_equal(c1, c2) for c1, c2 in zip(n1, n2))


class PackageXML(TEXML):
    class Status:
        new = 0
        exist = 1

    def __init__(self, description, mailto):
        self.root = etree.Element("package", version="1.0")

        self.description = etree.SubElement(self.root, "description")
        self.description.text = description
        self.author = etree.SubElement(self.root, "author", mailto=mailto)
        self.session = None

    def add_session_arg(self, name, value=None):
        self._create_session()

        def eq_node(node):
            return node.get("name") == name and node.get("value") == value

        arg = etree.Element("arg", name=name)
        if value:
            arg.set("value", value)

        return TEXML.addnext_unique(self.session, arg, eq_node)

    def add_run_script(self, script, name=None, args=None):
        run_attr = dict()
        attr = dict(name=script)
        if name:
            run_attr["name"] = name
        return self._add_run("script", run_attr, attr, args)

    def add_run_package(self, name=None, src=None, args=None):
        attr = dict()
        if src:
            attr["src"] = src
        if name:
            attr["name"] = name
        return self._add_run("package", dict(), attr, args)

    def _add_run(self, run_type, run_attr=None, attr=None, args=None):
        if run_attr is None:
            run_attr = dict()
        if attr is None:
            attr = dict()

        self._create_session()

        def eq_node(node):
            type_node = node.find(run_type)
            return node.attrib == run_attr and type_node.attrib == attr

        run = etree.Element("run", attrib=run_attr)
        etree.SubElement(run, run_type, attrib=attr)
        if isinstance(args, list):
            for name, value in args:
                if value:
                    etree.SubElement(run, "arg", name=name, value=value)
                else:
                    etree.SubElement(run, "arg", name=name)

        return TEXML.addnext_unique(self.session, run, eq_node)

    def _create_session(self):
        if self.session is None:
            self.session = etree.SubElement(self.root, "session")

    def tostring(self):
        pkg_bytes = etree.tostring(self.root, pretty_print=True,
                                   doctype="<?xml version=\"1.0\"?>")
        return str(pkg_bytes, encoding="utf-8")

    @staticmethod
    def load(filename):
        pkg = PackageXML(description="", mailto="")

        parser = etree.XMLParser(remove_blank_text=True)
        pkg.root = etree.parse(filename, parser).getroot()

        pkg.description = pkg.root.find("description")
        pkg.author = pkg.root.find("author")
        pkg.session = pkg.root.find("session")

        if pkg.description is None or pkg.author is None:
            raise RuntimeError("Failed load package.xml: '{}' "
                               "not have description or author", filename)

        return pkg

    def save(self, filename):
        if filename is None:
            self.root.dump()
        tree = etree.ElementTree(self.root)
        tree.write(filename, pretty_print=True)


class TRCResult(enum.Enum):
    PASSED = "PASSED"
    FAILED = "FAILED"
    SKIPPED = "SKIPPED"


class TRCTest(TEXML):
    def __init__(self, name, type, objective=None, notes=None):
        self.root = etree.Element("test", name=name, type=type)
        self.objective = etree.SubElement(self.root, "objective")
        self.notes = etree.SubElement(self.root, "notes")
        self.objective.text = objective
        self.notes.text = notes

        self.iter = None

    def add_test(self, name, type, result, objective=None, notes=None,
                 args=None):
        self._create_iter(result.value)
        test = etree.Element("test", name=name, type=type)

        obj = etree.SubElement(test, "objective")
        if objective:
            obj.text = objective
        if notes:
            nts = etree.SubElement(test, "notes")
            nts.text = notes

        iter_node = etree.SubElement(test, "iter", result=result.value)
        for arg_name, arg_value in args:
            arg = etree.SubElement(iter_node, "arg", name=arg_name)
            if arg_value:
                arg.set("value", arg_value)

        def test_eq(node):
            iter = node.find("iter")
            return node.get("name") == name and node.get("type") == type and \
                   TEXML.node_equal(iter, iter_node)

        return TEXML.addnext_unique(self.iter, test, test_eq)

    def include_file(self, path):
        self._create_iter(TRCResult.PASSED.value)
        url = "http://www.w3.org/2003/XInclude"
        include = etree.Element(f"{{{url}}}include", nsmap=dict(xi=url),
                                href=path, parse="xml")

        def include_eq(node):
            return node.get("href") == path

        return TEXML.addnext_unique(self.iter, include, include_eq)

    @staticmethod
    def load(filename ):
        trc = TRCTest("", "")

        parser = etree.XMLParser(remove_blank_text=True)
        trc.root = etree.parse(filename, parser).getroot()

        trc.objective = trc.root.find("objective")
        trc.notes = trc.root.find("notes")
        trc.iter = trc.root.find("iter")

        return trc

    def save(self, filename):
        if filename is None:
            self.root.dump()
        tree = etree.ElementTree(self.root)
        tree.write(filename, pretty_print=True)

    def _create_iter(self, result):
        if self.iter is None:
            self.iter = etree.SubElement(self.root, "iter", result=result)
