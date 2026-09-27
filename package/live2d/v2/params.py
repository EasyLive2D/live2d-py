class Parameter:
    TYPE_INNER = 0  # params defined in moc file
    TYPE_OUTER = 1  # params generated from motion file

    def __init__(self):
        self.id: str = ""
        self.type: int = 0
        self.value: float = 0
        self.max: float = 0
        self.min: float = 0
        self.default: float = 0

    def __str__(self):
        return f"Parameter(id={self.id}, type={self.type}, value={self.value}, max={self.max}, min={self.min}, default={self.default})"
