"""Small TAC evaluator for control-flow regression tests."""
import ast
import re


def execute(text, parameters=None, inputs=()):
    parameters = parameters or {}
    functions = {}
    globals_ = {}
    current = None
    for raw in text.splitlines():
        line = raw.strip()
        if line.startswith('global '):
            globals_[line.split()[1].rstrip(',')] = 0
        elif line.startswith('beginFunc '):
            current = []
            functions[line.split()[1]] = current
        elif line.startswith('endFunc '):
            current = None
        elif current is not None:
            current.append(line)
    output = []
    input_values = iter(inputs)
    arrays = {}
    steps = 0

    def call(name, arguments):
        nonlocal steps
        code = functions[name]
        labels = {line[:-1]: i for i, line in enumerate(code) if line.endswith(':')}
        env = dict(zip(parameters.get(name, ()), arguments))
        pending = []

        def value(address):
            if address in env:
                return env[address]
            if address in globals_:
                return globals_[address]
            return ast.literal_eval(address)

        def store(name, item):
            if name in globals_:
                globals_[name] = item
            else:
                env[name] = item

        pc = 0
        while pc < len(code):
            steps += 1
            if steps > 10000:
                raise RuntimeError('TAC execution exceeded step limit')
            line = code[pc]
            pc += 1
            if line.endswith(':'):
                continue
            if line.startswith('local '):
                env[line.split()[1].rstrip(',')] = 0
            elif line.startswith('goto '):
                pc = labels[line.split()[1]]
            elif line.startswith(('ifTrue ', 'ifFalse ')):
                operation, operand, _, label = line.split()
                if bool(value(operand)) == (operation == 'ifTrue'):
                    pc = labels[label]
            elif line.startswith('print '):
                output.append(value(line[6:]))
            elif line.startswith('read '):
                store(line[5:], next(input_values))
            elif line.startswith('param '):
                pending.append(value(line[6:]))
            elif line == 'return':
                return None
            elif line.startswith('return '):
                return value(line[7:])
            elif 'call ' in line:
                match = re.fullmatch(r'(?:(\w+) = )?call (\w+) (\d+)', line)
                if not match:
                    raise ValueError(line)
                destination, callee, count = match.groups()
                count = int(count)
                arguments = pending[-count:] if count else []
                if count:
                    del pending[-count:]
                result = call(callee, arguments)
                if destination:
                    store(destination, result)
            else:
                left, right = line.split(' = ', 1)
                indexed = re.fullmatch(r'(\w+)\[(.+)\]', left)
                if indexed:
                    array, index = indexed.groups()
                    arrays.setdefault(array, {})[value(index)] = value(right)
                    continue
                indexed = re.fullmatch(r'(\w+)\[(.+)\]', right)
                if indexed:
                    array, index = indexed.groups()
                    result = arrays[array][value(index)]
                elif right.startswith('!'):
                    result = int(not value(right[1:]))
                elif right.startswith('-') and not re.fullmatch(r'-\d+(?:\.\d+)?', right):
                    result = -value(right[1:])
                else:
                    binary = re.fullmatch(r'(\S+) ([+*/%<>-]|<=|>=|==|!=|&&|\|\|) (\S+)', right)
                    if binary:
                        a, operation, b = binary.groups()
                        a, b = value(a), value(b)
                        operations = {
                            '+': lambda: a+b, '-': lambda: a-b, '*': lambda: a*b,
                            '/': lambda: int(a/b) if isinstance(a,int) and isinstance(b,int) else a/b,
                            '%': lambda: a-int(a/b)*b,
                            '<': lambda: int(a<b), '>': lambda: int(a>b),
                            '<=': lambda: int(a<=b), '>=': lambda: int(a>=b),
                            '==': lambda: int(a==b), '!=': lambda: int(a!=b),
                            '&&': lambda: int(bool(a) and bool(b)),
                            '||': lambda: int(bool(a) or bool(b)),
                        }
                        result = operations[operation]()
                    else:
                        result = value(right)
                store(left, result)
        return None

    call('main', [])
    return output
