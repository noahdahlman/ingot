from conan import ConanFile
from conan.tools.meson import Meson, MesonToolchain
from conan.tools.gnu import PkgConfigDeps


class IngotConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = "doctest/2.4.11"

    def generate(self):
        tc = MesonToolchain(self)
        tc.project_options["wrap_mode"] = "default"
        tc.generate()
        deps = PkgConfigDeps(self)
        deps.generate()

    def build(self):
        meson = Meson(self)
        meson.configure()
        meson.build()
