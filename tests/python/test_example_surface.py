import ast
from pathlib import Path

import fourier_aurora_client as sdk


PROJECT_ROOT = Path(__file__).resolve().parents[2]


def test_python_recipes_are_standalone_and_compile():
    recipes = sorted((PROJECT_ROOT / "examples" / "gr3" / "python").glob("*.py"))
    assert [recipe.name for recipe in recipes] == [
        "get_info.py",
        "swing_arms.py",
        "walk_forward.py",
    ]
    for recipe in recipes:
        source = recipe.read_text(encoding="utf-8")
        compile(source, str(recipe), "exec")
        tree = ast.parse(source)
        imported_modules = {
            node.module
            for node in ast.walk(tree)
            if isinstance(node, ast.ImportFrom)
        }
        assert "example_common" not in imported_modules


def test_cpp_recipes_do_not_include_shared_example_helpers():
    recipes = sorted((PROJECT_ROOT / "examples" / "gr3" / "cpp").glob("*.cpp"))
    assert [recipe.name for recipe in recipes] == [
        "get_info.cpp",
        "swing_arms.cpp",
        "walk_forward.cpp",
    ]
    for recipe in recipes:
        source = recipe.read_text(encoding="utf-8")
        assert "example_common" not in source
        assert "client_common" not in source


def test_recipe_python_api_surface_exists():
    required = (
        "AuroraClient",
        "ControlGroupCommand",
        "Endpoint",
        "JointCommand",
        "JointControlType",
        "MotorMode",
    )
    for name in required:
        assert hasattr(sdk, name)

    client = sdk.AuroraClient.get_instance()
    for name in (
        "config",
        "configure",
        "start",
        "stop",
        "wait_for_endpoints",
        "wait_for_fsm_state",
        "get_aurora_state",
        "get_estimator_data",
        "get_control_group_state",
        "request_fsm_state",
        "set_velocity",
        "publish_control_group_command",
    ):
        assert hasattr(client, name)
